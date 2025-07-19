#include <stdexcept>

#include <utility.hpp>

namespace utility {

    [[nodiscard]] std::string parse_imsi_from_bcd(const std::vector<uint8_t> &packet) {
        if (packet.size() < 4) {
            throw std::invalid_argument("Packet too short for IMSI");
        }

        if (packet[0] != 1) {
            throw std::invalid_argument("Invalid IMSI type field");
        }

        uint16_t length = (static_cast<uint16_t>(packet[1]) << 8) | packet[2];

        size_t declared_length = 3 + length;
        if (packet.size() < declared_length) {
            throw std::invalid_argument("Packet size doesn't match declared length");
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
                throw std::invalid_argument("Invalid BCD digit in IMSI");
            }

            if (digit2 <= 9) {
                imsi.push_back('0' + digit2);
            } else if (digit2 == 0x0F) {
                break;
            } else {
                throw std::invalid_argument("Invalid BCD digit in IMSI");
            }
        }

        if (imsi.length() > 15 or imsi.length() < 6) {
            throw std::invalid_argument("Invalid IMSI length");
        }

        return imsi;
    }

} // namespace utility
