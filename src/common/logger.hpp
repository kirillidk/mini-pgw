#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <boost/log/trivial.hpp>

class config;

class logger_exception : public std::runtime_error {
public:
    explicit logger_exception(const std::string &message) : std::runtime_error("logger_exception: " + message) {}
};

class logger {
public:
    using log_level = boost::log::trivial::severity_level;

public:
    explicit logger(std::shared_ptr<config> config);
    ~logger() { BOOST_LOG_TRIVIAL(info) << "Logger destroyed"; }

    logger(const logger &) = delete;
    logger &operator=(const logger &) = delete;
    logger(logger &&) = delete;
    logger &operator=(logger &&) = delete;

    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);
    void fatal(std::string_view message);

    void log(log_level level, std::string_view message);

private:
    void setup(const std::filesystem::path &log_file, const std::string &log_level_str);
    log_level parse_log_level(const std::string &level_str);

private:
    std::shared_ptr<config> _config;
    boost::log::sources::severity_logger<log_level> _logger;
    log_level _min_level;
};
