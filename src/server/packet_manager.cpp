#include <string>
#include <thread>

#include <config.hpp>
#include <packet_manager.hpp>
#include <session_manager.hpp>
#include <thread_pool.hpp>
#include <utility.hpp>

packet_manager::packet_manager(std::shared_ptr<session_manager> sm, std::shared_ptr<thread_pool> tp,
                               std::shared_ptr<config> cfg) :
    _session_manager(std::move(sm)), _thread_pool(std::move(tp)), _config(std::move(cfg)) {}

std::expected<std::string, packet_manager_error> packet_manager::handle_packet(const Packet &packet) {
    std::expected<std::string, utility::parse_error> imsi = utility::parse_imsi_from_bcd(packet);

    if (not imsi.has_value()) {
        // todo log imsi.error()
        return std::unexpected(packet_manager_error::packet_parsing_failed);
    }

    std::shared_ptr result = _session_manager->create_session(imsi.value());
    if (not result) {
        return "rejected";
    }

    _thread_pool->enqueue([this, imsi]() {
        std::chrono::seconds timeout = std::chrono::seconds(_config->get_session_timeout_sec().value());
        std::this_thread::sleep_for(timeout);
        _session_manager->delete_session(imsi.value());
    });

    return "created";
}
