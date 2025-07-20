#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class config;
class session;

class control_plane {
public:
    explicit control_plane(config &config);

    std::shared_ptr<session> create_session(std::string imsi);
    config &get_config() const;

private:
    config &_config;
    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
};
