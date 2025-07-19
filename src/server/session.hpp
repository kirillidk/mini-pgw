#pragma once

#include <memory>
#include <string>

class session {
public:
    static std::shared_ptr<session> create(std::string imsi);

private:
    session(std::string imsi);

    std::string _imsi;
};
