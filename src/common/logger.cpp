#include <config.hpp>
#include <logger.hpp>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>

logger::logger(std::shared_ptr<config> config) : _config(std::move(config)) {
    auto log_file = _config->get_log_file();
    auto log_level = _config->get_log_level();

    if (!log_file.has_value()) {
        throw logger_exception("Log file path not specified in config");
    }

    if (!log_level.has_value()) {
        throw logger_exception("Log level not specified in config");
    }

    setup(log_file.value(), log_level.value());
}

void logger::setup(const std::filesystem::path &log_file, const std::string &log_level_str) {
    _min_level = parse_log_level(log_level_str);

    boost::log::add_common_attributes();

    auto file_sink = boost::log::add_file_log(boost::log::keywords::file_name = log_file.string(),
                                              boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%",
                                              boost::log::keywords::auto_flush = true,
                                              boost::log::keywords::open_mode = std::ios::out | std::ios::app);

    auto console_sink = boost::log::add_console_log(std::clog, boost::log::keywords::format =
                                                                       "[%TimeStamp%] [%Severity%] %Message%");

    boost::log::core::get()->set_filter(boost::log::trivial::severity >= _min_level);
}

logger::log_level logger::parse_log_level(const std::string &level_str) {
    if (boost::iequals(level_str, "debug"))
        return log_level::debug;
    if (boost::iequals(level_str, "info"))
        return log_level::info;
    if (boost::iequals(level_str, "warning"))
        return log_level::warning;
    if (boost::iequals(level_str, "error"))
        return log_level::error;
    if (boost::iequals(level_str, "fatal"))
        return log_level::fatal;

    throw logger_exception("Invalid log level: " + level_str);
}

void logger::debug(std::string_view message) { log(log_level::debug, message); }

void logger::info(std::string_view message) { log(log_level::info, message); }

void logger::warning(std::string_view message) { log(log_level::warning, message); }

void logger::error(std::string_view message) { log(log_level::error, message); }

void logger::fatal(std::string_view message) { log(log_level::fatal, message); }

void logger::log(log_level level, std::string_view message) {
    if (level >= _min_level) {
        BOOST_LOG_SEV(_logger, level) << message;
    }
}
