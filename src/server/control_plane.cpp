#include <control_plane.hpp>
#include <session.hpp>

control_plane::control_plane(config &config, thread_pool &tp) : _config(config), _thread_pool(tp) {}

std::shared_ptr<session> control_plane::create_session(const std::string &imsi) {
    if (_sessions.contains(imsi)) {
        return nullptr;
    }

    _sessions[imsi] = session::create(imsi);
    return _sessions[imsi];
}

void control_plane::delete_session(const std::string &imsi) { _sessions.erase(imsi); }

config &control_plane::get_config() const { return _config; }

thread_pool &control_plane::get_thread_pool() const { return _thread_pool; }
