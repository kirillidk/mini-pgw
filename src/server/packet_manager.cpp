#include <string>
#include <thread>

#include <packet_manager.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <session.hpp>
#include <session_manager.hpp>
#include <thread_pool.hpp>
#include <utility.hpp>

packet_manager::packet_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                               std::shared_ptr<session_manager> session_manager) :
    _config(std::move(config)), _event_bus(std::move(event_bus)), _session_manager(std::move(session_manager)) {}

std::expected<std::string, packet_manager_error> packet_manager::handle_packet(const Packet &packet) {
    std::expected<std::string, utility::parse_error> imsi = utility::parse_imsi_from_bcd(packet);

    if (not imsi.has_value()) {
        // todo log imsi.error()
        return std::unexpected(packet_manager_error::packet_parsing_failed);
    }

    std::string imsi_str = imsi.value();

    if (_session_manager->in_blacklist(imsi_str)) {
        _event_bus->publish<events::reject_session_event>(imsi_str);
        return "rejected";
    }

    std::shared_ptr<session> result = _session_manager->create_session(imsi_str);

    if (result) {
        _event_bus->publish<events::create_session_event>(imsi_str);
        return "created";
    } else {
        _event_bus->publish<events::reject_session_event>(imsi_str);
        return "rejected";
    }
}
