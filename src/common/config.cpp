#include <config.hpp>
#include <fstream>

config::config(const std::filesystem::path &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw config_exception("Cannot open config file: " + path.string());
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error &e) {
        throw config_exception("Invalid JSON: " + std::string(e.what()));
    }

    _ip = extract_value<std::string>(json_data, "server_ip");
    _port = extract_value<uint32_t>(json_data, "server_port");
    _http_port = extract_value<uint32_t>(json_data, "http_port");
    _session_timeout_sec = extract_value<uint32_t>(json_data, "session_timeout_sec");
    _cdr_file = extract_value<std::filesystem::path>(json_data, "cdr_file");
    _graceful_shutdown_rate = extract_value<uint32_t>(json_data, "graceful_shutdown_rate");
    _log_file = extract_value<std::filesystem::path>(json_data, "log_file");
    _log_level = extract_value<std::string>(json_data, "log_level");
    _blacklist = extract_value<std::unordered_set<std::string>>(json_data, "blacklist");
}

template<typename T>
std::optional<T> config::extract_value(const nlohmann::json &json, std::string_view key) {
    if (json.contains(key) && !json[key].is_null()) {
        return json[key].get<T>();
    }
    return std::nullopt;
}

std::optional<std::string> config::get_ip() const { return _ip; }

std::optional<uint32_t> config::get_port() const { return _port; }

std::optional<uint32_t> config::get_http_port() const { return _http_port; }

std::optional<uint32_t> config::get_session_timeout_sec() const { return _session_timeout_sec; }

std::optional<std::filesystem::path> config::get_cdr_file() const { return _cdr_file; }

std::optional<uint32_t> config::get_graceful_shutdown_rate() const { return _graceful_shutdown_rate; }

std::optional<std::filesystem::path> config::get_log_file() const { return _log_file; }

std::optional<std::string> config::get_log_level() const { return _log_level; }

std::optional<std::unordered_set<std::string>> config::get_blacklist() const { return _blacklist; }