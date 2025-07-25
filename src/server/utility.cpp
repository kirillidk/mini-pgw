#include <stdexcept>

#include <utility.hpp>

#include <chrono>
#include <format>

namespace utility {

    std::expected<std::string, parse_error> parse_imsi_from_bcd(std::span<const uint8_t> packet) {
        if (packet.size() < 4) {
            return std::unexpected(parse_error::packet_too_short);
        }

        if (packet[0] != 1) {
            return std::unexpected(parse_error::invalid_imsi_type);
        }

        uint16_t length = (static_cast<uint16_t>(packet[1]) << 8) | packet[2];

        size_t declared_length = 3 + length;
        if (packet.size() < declared_length) {
            return std::unexpected(parse_error::packet_size_mismatch);
        }


        std::string imsi{};
        imsi.reserve(declared_length * 2);

        for (size_t i = 4; i < packet.size(); ++i) {
            uint8_t byte = packet[i];

            uint8_t digit1 = byte & 0x0F;
            uint8_t digit2 = (byte >> 4) & 0x0F;

            if (digit1 <= 9) {
                imsi.push_back('0' + digit1);
            } else {
                return std::unexpected(parse_error::invalid_bcd_digit);
            }

            if (digit2 <= 9) {
                imsi.push_back('0' + digit2);
            } else if (digit2 == 0x0F) {
                break;
            } else {
                return std::unexpected(parse_error::invalid_bcd_digit);
            }
        }

        if (imsi.length() > 15 or imsi.length() < 6) {
            return std::unexpected(parse_error::invalid_imsi_length);
        }

        return imsi;
    }

    std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto ts = std::chrono::floor<std::chrono::seconds>(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - ts).count();
        auto lt = std::chrono::current_zone()->to_local(ts);

        return std::format("{:%Y-%m-%d %H:%M:%S}.{:06}", lt, ms);
    }

} // namespace utility
