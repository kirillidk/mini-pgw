#include <control_plane.hpp>
#include <session.hpp>

control_plane::control_plane(config &config) : _config(config) {}

std::shared_ptr<session> control_plane::create_session(std::string imsi) {
    if (not _sessions.contains(imsi)) {
        auto session_ptr = session::create(imsi);
        _sessions[imsi] = session_ptr;
    }

    return _sessions[imsi];
}

config &control_plane::get_config() const { return _config; }
