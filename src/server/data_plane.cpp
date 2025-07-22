#include <string>
#include <thread>

#include <config.hpp>
#include <data_plane.hpp>
#include <thread_pool.hpp>
#include <utility.hpp>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

std::expected<std::string, data_plane_error> data_plane::handle_packet(const Packet &packet) {
    std::expected<std::string, utility::parse_error> imsi = utility::parse_imsi_from_bcd(packet);

    if (not imsi.has_value()) {
        // todo log imsi.error()
        return std::unexpected(data_plane_error::packet_parsing_failed);
    }

    std::shared_ptr result = _control_plane.create_session(imsi.value());
    if (not result) {
        return "rejected";
    }

    _control_plane.get_thread_pool().enqueue([this, imsi]() {
        std::chrono::seconds timeout =
                std::chrono::seconds(_control_plane.get_config().get_session_timeout_sec().value());
        std::this_thread::sleep_for(timeout);
        _control_plane.delete_session(imsi.value());
    });

    return "created";
}
