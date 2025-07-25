#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <string>

class config;
class event_bus;
class logger;

enum class cdr_action { created, deleted, rejected };

struct cdr_record {
    std::string timestamp;
    std::string imsi;
    cdr_action action;
};

class cdr_writer_exception : public std::runtime_error {
public:
    explicit cdr_writer_exception(const std::string &message) :
        std::runtime_error("cdr_writer_exception: " + message) {}
};

class cdr_writer {
public:
    explicit cdr_writer(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                        std::shared_ptr<logger> logger);
    ~cdr_writer();

    cdr_writer(const cdr_writer &) = delete;
    cdr_writer &operator=(const cdr_writer &) = delete;
    cdr_writer(cdr_writer &&) = delete;
    cdr_writer &operator=(cdr_writer &&) = delete;

    void write_record(const cdr_record &record);

private:
    void setup();

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<logger> _logger;

    std::ofstream _file;
    std::mutex _file_mutex;
};
