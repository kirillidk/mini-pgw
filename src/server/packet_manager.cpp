#include <string>
#include <thread>

#include <packet_manager.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <logger.hpp>
#include <session.hpp>
#include <session_manager.hpp>
#include <thread_pool.hpp>
#include <utility.hpp>

#include <magic_enum/magic_enum.hpp>

packet_manager::packet_manager(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus,
                               std::shared_ptr<session_manager> session_manager, std::shared_ptr<logger> logger) :
    _config(std::move(config)), _event_bus(std::move(event_bus)), _session_manager(std::move(session_manager)),
    _logger(std::move(logger)) {
    _logger->info("Packet manager initialized");
}

packet_manager::~packet_manager() { _logger->info("Packet manager is destroyed"); }

std::expected<std::string, packet_manager_error> packet_manager::handle_packet(Packet packet) {
    _logger->debug("Handling packet of size: " + std::to_string(packet.size()));

    std::expected<std::string, utility::parse_error> imsi = utility::parse_imsi_from_bcd(packet);

    if (not imsi.has_value()) {
        std::string error_msg = "Failed to parse IMSI: " + std::string(magic_enum::enum_name(imsi.error()));
        _logger->warning(error_msg);

        return std::unexpected(packet_manager_error::packet_parsing_failed);
    }

    std::string imsi_str = imsi.value();
    _logger->debug("Extracted IMSI: " + imsi_str);

    if (_session_manager->has_blacklist_session(imsi_str)) {
        _logger->info("IMSI " + imsi_str + " is in blacklist, rejecting session");

        _event_bus->publish<events::reject_session_event>(imsi_str);
        return "rejected";
    }

    std::shared_ptr<session> result = _session_manager->create_session(imsi_str);

    if (result) {
        _logger->info("Session created for IMSI: " + imsi_str);

        _event_bus->publish<events::create_session_event>(imsi_str);
        return "created";
    } else {
        _logger->warning("Failed to create session for IMSI: " + imsi_str + " (session already exists)");

        _event_bus->publish<events::reject_session_event>(imsi_str);
        return "rejected";
    }
}
