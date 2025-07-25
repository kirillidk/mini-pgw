#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

class session;
class event_bus;
class thread_pool;
class config;
class logger;

class session_manager {
public:
    session_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                    std::shared_ptr<thread_pool> thread_pool, std::shared_ptr<logger> logger);

public:
    [[nodiscard]] std::shared_ptr<session> create_session(const std::string &imsi);
    void delete_session(const std::string &imsi);

    [[nodiscard]] bool in_blacklist(const std::string &imsi) const;

private:
    void setup();

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<thread_pool> _thread_pool;
    std::shared_ptr<logger> _logger;

    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
    std::unordered_set<std::string> _blacklist;

    std::mutex _sessions_mutex;
};
