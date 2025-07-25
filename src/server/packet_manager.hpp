#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>

class session_manager;
class thread_pool;
class config;
class event_bus;
class logger;

enum class packet_manager_error {
    packet_parsing_failed,
};

class packet_manager {
public:
    using Packet = std::vector<uint8_t>;

    explicit packet_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                            std::shared_ptr<session_manager> session_manager, std::shared_ptr<logger> logger);

public:
    std::expected<std::string, packet_manager_error> handle_packet(const Packet &packet);

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<session_manager> _session_manager;
    std::shared_ptr<logger> _logger;
};