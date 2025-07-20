#include <string>

#include <data_plane.hpp>
#include <utility.hpp>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

std::expected<std::string, data_plane_error> data_plane::handle_packet(const Packet &packet) {
    std::expected<std::string, utility::parse_error> imsi = utility::parse_imsi_from_bcd(packet);

    if (not imsi.has_value()) {
        // todo log imsi.error()
        return std::unexpected(data_plane_error::packet_parsing_failed);
    }

    _control_plane.create_session(imsi.value());

    return "created";
}
