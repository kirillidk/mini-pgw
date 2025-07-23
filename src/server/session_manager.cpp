#include <session_manager.hpp>

#include <session.hpp>

std::shared_ptr<session> session_manager::create_session(const std::string &imsi) {
    if (_sessions.contains(imsi)) {
        return nullptr;
    }

    _sessions[imsi] = session::create(imsi);
    return _sessions[imsi];
}

void session_manager::delete_session(const std::string &imsi) { _sessions.erase(imsi); }
