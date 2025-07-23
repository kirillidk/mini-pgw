#pragma once

#include <memory>
#include <string>

class session {
public:
    static std::shared_ptr<session> create(std::string imsi);

    std::string get_imsi() const;

private:
    session(std::string imsi);

    std::string _imsi;
};
