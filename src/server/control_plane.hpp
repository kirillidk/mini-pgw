#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class config;
class session;
class thread_pool;

class control_plane {
public:
    explicit control_plane(config &config, thread_pool &tp);

    std::shared_ptr<session> create_session(const std::string &imsi);
    void delete_session(const std::string &imsi);

    config &get_config() const;
    thread_pool &get_thread_pool() const;

private:
    config &_config;
    thread_pool &_thread_pool;
    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
};
