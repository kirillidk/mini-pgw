#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class session;

class session_manager {
public:
    session_manager() = default;

    std::shared_ptr<session> create_session(const std::string &imsi);
    void delete_session(const std::string &imsi);

private:
    std::unordered_map<std::string, std::shared_ptr<session>> _sessions;
};