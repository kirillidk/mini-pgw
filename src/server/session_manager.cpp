#include <session_manager.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <session.hpp>
#include <thread_pool.hpp>

session_manager::session_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                                 std::shared_ptr<thread_pool> thread_pool) :
    _config(std::move(config)), _event_bus(std::move(event_bus)), _thread_pool(std::move(thread_pool)),
    _blacklist(_config->get_blacklist().value()) {
    setup();
}

void session_manager::setup() {
    _event_bus->subscribe<events::create_session_event>([this](std::string imsi) {
        _thread_pool->enqueue([this, imsi = std::move(imsi)]() {
            std::chrono::seconds timeout = std::chrono::seconds(_config->get_session_timeout_sec().value());
            std::this_thread::sleep_for(timeout);

            delete_session(imsi);
            _event_bus->publish<events::delete_session_event>(imsi);
        });
    });
}

std::shared_ptr<session> session_manager::create_session(const std::string &imsi) {
    if (_sessions.contains(imsi)) {
        return nullptr;
    }

    _sessions[imsi] = session::create(imsi);
    return _sessions[imsi];
}

void session_manager::delete_session(const std::string &imsi) { _sessions.erase(imsi); }

bool session_manager::in_blacklist(const std::string &imsi) const { return _blacklist.contains(imsi); }
