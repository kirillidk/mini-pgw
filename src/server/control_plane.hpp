#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <session.hpp>

class control_plane {
public:
    std::shared_ptr<session> create_session(std::string imsi);

private:
    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
};
