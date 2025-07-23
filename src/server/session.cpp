#include <session.hpp>

std::shared_ptr<session> session::create(std::string imsi) {
    return std::shared_ptr<session>(new session(std::move(imsi)));
}

std::string session::get_imsi() const { return _imsi; }

session::session(std::string imsi) : _imsi(std::move(imsi)) {}
