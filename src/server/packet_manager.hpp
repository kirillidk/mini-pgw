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

enum class packet_manager_error {
    packet_parsing_failed,
};

class packet_manager {
public:
    using Packet = std::vector<uint8_t>;

    explicit packet_manager(std::shared_ptr<session_manager> sm, std::shared_ptr<thread_pool> tp,
                            std::shared_ptr<config> cfg, std::shared_ptr<event_bus> eb);

public:
    std::expected<std::string, packet_manager_error> handle_packet(const Packet &packet);

private:
    std::shared_ptr<session_manager> _session_manager;
    std::shared_ptr<thread_pool> _thread_pool;
    std::shared_ptr<config> _config;
    std::shared_ptr<event_bus> _event_bus;
};
