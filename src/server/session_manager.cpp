#include <session_manager.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <logger.hpp>
#include <session.hpp>

session_manager::session_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                                 std::shared_ptr<logger> logger) :
    _config(std::move(config)), _event_bus(std::move(event_bus)), _logger(std::move(logger)),
    _blacklist(_config->get_blacklist().value()) {

    _logger->info("Session manager initialized with " + std::to_string(_blacklist.size()) + " blacklisted IMSIs");
    setup_event_handlers();
}

session_manager::~session_manager() { _logger->debug("Session manager is destroyed"); }

void session_manager::setup_event_handlers() {
    _logger->info("Setting up session manager event handlers");

    _event_bus->subscribe<events::create_session_event>([this](std::string imsi) {
        _logger->debug("Scheduling session deletion for IMSI: " + imsi);

        std::chrono::seconds timeout = std::chrono::seconds(_config->get_session_timeout_sec().value());

        _logger->debug("Session for IMSI " + imsi + " will expire in " + std::to_string(timeout.count()) + " seconds");

        std::this_thread::sleep_for(timeout);

        delete_session(imsi);
        _logger->info("Session expired for IMSI: " + imsi);

        _event_bus->publish<events::delete_session_event>(imsi);
    });

    _event_bus->subscribe<events::graceful_shutdown_event>([this]() { graceful_shutdown_worker(); });

    _logger->info("Session manager setup of event handlers is completed");
}

std::shared_ptr<session> session_manager::create_session(const std::string &imsi) {
    std::lock_guard<std::mutex> lock(_sessions_mutex);

    if (_sessions.contains(imsi)) {
        _logger->debug("Session creation failed - IMSI already exists: " + imsi);
        return nullptr;
    }

    _sessions[imsi] = session::create(imsi);
    _logger->debug("Session created successfully for IMSI: " + imsi +
                   " (total sessions: " + std::to_string(_sessions.size()) + ")");

    return _sessions[imsi];
}

void session_manager::delete_session(const std::string &imsi) {
    std::lock_guard<std::mutex> lock(_sessions_mutex);

    auto it = _sessions.find(imsi);
    if (it != _sessions.end()) {
        _sessions.erase(it);
        _logger->debug("Session deleted for IMSI: " + imsi +
                       " (remaining sessions: " + std::to_string(_sessions.size()) + ")");
    } else {
        _logger->warning("Attempted to delete non-existent session for IMSI: " + imsi);
    }
}

bool session_manager::has_blacklist_session(const std::string &imsi) const {
    std::lock_guard<std::mutex> lock(_sessions_mutex);

    bool is_blacklisted = _blacklist.contains(imsi);
    if (is_blacklisted) {
        _logger->debug("IMSI " + imsi + " found in blacklist");
    }
    return is_blacklisted;
}

bool session_manager::has_active_session(const std::string &imsi) const {
    std::lock_guard<std::mutex> lock(_sessions_mutex);

    bool is_active = _sessions.contains(imsi);
    if (is_active) {
        _logger->debug("IMSI " + imsi + " is active");
    }
    return is_active;
}

void session_manager::graceful_shutdown_worker() {
    if (_shutdown_requested.exchange(true)) {
        _logger->warning("Graceful shutdown already requested, ignoring duplicate request");
        return;
    }

    _logger->info("Starting graceful shutdown worker");

    auto shutdown_rate = _config->get_graceful_shutdown_rate().value();
    auto delay_ms = std::chrono::milliseconds(1000 / shutdown_rate);

    _logger->info("Graceful shutdown rate: " + std::to_string(shutdown_rate) + " sessions per second");

    while (true) {
        std::string imsi_to_delete;

        {
            std::lock_guard<std::mutex> lock(_sessions_mutex);
            if (_sessions.empty()) {
                _logger->info("All sessions have been gracefully removed");
                break;
            }

            imsi_to_delete = _sessions.begin()->first;
        }

        delete_session(imsi_to_delete);
        _logger->info("Gracefully removed session for IMSI: " + imsi_to_delete);

        _event_bus->publish<events::delete_session_event>(imsi_to_delete);

        std::this_thread::sleep_for(delay_ms);
    }

    _logger->info("Graceful shutdown completed - all sessions removed");
}
