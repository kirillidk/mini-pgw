#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class session;
class event_bus;
class thread_pool;
class config;

class session_manager {
public:
    session_manager(std::shared_ptr<event_bus> event_bus, std::shared_ptr<thread_pool> thread_pool,
                    std::shared_ptr<config> config);

public:
    std::shared_ptr<session> create_session(const std::string &imsi);
    void delete_session(const std::string &imsi);

private:
    void setup();

    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<thread_pool> _thread_pool;
    std::shared_ptr<config> _config;
};
