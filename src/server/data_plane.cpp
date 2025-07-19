#include <stdexcept>
#include <string>

#include <data_plane.hpp>
#include <utility.hpp>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

std::string data_plane::handle_packet(const Packet &packet) {
    std::string imsi = utility::parse_imsi_from_bcd(packet);

    std::shared_ptr<session> session_ptr = _control_plane.create_session(imsi);

    if (session_ptr) {
        return "created";
    } else {
        return "rejected";
    };
}
