#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>

class config_exception : public std::runtime_error {
public:
    explicit config_exception(const std::string &message) : std::runtime_error("config_exception: " + message) {}
};

class config {
public:
    explicit config(const std::filesystem::path &path);

    [[nodiscard]] std::optional<std::string> get_ip() const;
    [[nodiscard]] std::optional<uint32_t> get_port() const;
    [[nodiscard]] std::optional<uint32_t> get_session_timeout_sec() const;
    [[nodiscard]] std::optional<std::filesystem::path> get_cdr_file() const;
    [[nodiscard]] std::optional<uint32_t> get_graceful_shutdown_rate() const;
    [[nodiscard]] std::optional<std::filesystem::path> get_log_file() const;
    [[nodiscard]] std::optional<std::string> get_log_level() const;
    [[nodiscard]] std::optional<std::unordered_set<std::string>> get_blacklist() const;

private:
    template<typename T>
    std::optional<T> extract_value(const nlohmann::json &json, std::string_view key);

private:
    std::optional<std::string> _ip;
    std::optional<uint32_t> _port;
    std::optional<uint32_t> _session_timeout_sec;
    std::optional<std::filesystem::path> _cdr_file;
    std::optional<uint32_t> _graceful_shutdown_rate;
    std::optional<std::filesystem::path> _log_file;
    std::optional<std::string> _log_level;
    std::optional<std::unordered_set<std::string>> _blacklist;
};
