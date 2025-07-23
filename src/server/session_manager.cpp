#include <session_manager.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <session.hpp>
#include <thread_pool.hpp>

session_manager::session_manager(std::shared_ptr<event_bus> event_bus, std::shared_ptr<thread_pool> thread_pool,
                                 std::shared_ptr<config> config) :
    _event_bus(event_bus), _thread_pool(thread_pool), _config(config) {
    setup();
}

void session_manager::setup() {
    _event_bus->subscribe<events::delete_session_event>([this](const std::shared_ptr<session> &session) {
        _thread_pool->enqueue([this, session]() {
            std::chrono::seconds timeout = std::chrono::seconds(_config->get_session_timeout_sec().value());
            std::this_thread::sleep_for(timeout);
            delete_session(session->get_imsi());
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
