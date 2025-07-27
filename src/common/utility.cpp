#include <stdexcept>

#include <utility.hpp>

#include <algorithm>
#include <chrono>
#include <format>

namespace utility {

    std::expected<std::string, decode_error> decode_imsi_from_bcd(std::span<const uint8_t> packet) {
        if (packet.size() < 4) {
            return std::unexpected(decode_error::packet_too_short);
        }

        if (packet[0] != 1) {
            return std::unexpected(decode_error::invalid_imsi_type);
        }

        uint16_t length = (static_cast<uint16_t>(packet[1]) << 8) | packet[2];

        size_t declared_length = 3 + length;
        if (packet.size() < declared_length) {
            return std::unexpected(decode_error::packet_size_mismatch);
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
                return std::unexpected(decode_error::invalid_bcd_digit);
            }

            if (digit2 <= 9) {
                imsi.push_back('0' + digit2);
            } else if (digit2 == 0x0F) {
                break;
            } else {
                return std::unexpected(decode_error::invalid_bcd_digit);
            }
        }

        if (imsi.length() > 15 or imsi.length() < 6) {
            return std::unexpected(decode_error::invalid_imsi_length);
        }

        return imsi;
    }

    std::expected<std::vector<uint8_t>, encode_error> encode_imsi_to_bcd(const std::string &imsi) {
        if (!is_valid_imsi(imsi)) {
            return std::unexpected(encode_error::invalid_imsi_format);
        }

        std::vector<uint8_t> bcd_data;

        bcd_data.push_back(0x01);

        size_t bcd_bytes_needed = (imsi.length() + 1) / 2;

        uint16_t length = static_cast<uint16_t>(bcd_bytes_needed + 1);
        bcd_data.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
        bcd_data.push_back(static_cast<uint8_t>(length & 0xFF));

        bcd_data.push_back(0x00);

        std::string padded_imsi = imsi;
        if (padded_imsi.length() % 2 != 0) {
            padded_imsi += 'F';
        }

        for (size_t i = 0; i < padded_imsi.length(); i += 2) {
            uint8_t digit1 = padded_imsi[i] - '0';
            uint8_t digit2;

            if (i + 1 < padded_imsi.length() && padded_imsi[i + 1] != 'F') {
                digit2 = padded_imsi[i + 1] - '0';
            } else {
                digit2 = 0x0F;
            }

            uint8_t bcd_byte = (digit2 << 4) | digit1;
            bcd_data.push_back(bcd_byte);
        }

        return bcd_data;
    }

    bool is_valid_imsi(const std::string &imsi) {
        if (imsi.empty() || imsi.length() < 6 || imsi.length() > 15) {
            return false;
        }

        return std::ranges::all_of(imsi, [](char c) { return std::isdigit(c); });
    }

    std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto ts = std::chrono::floor<std::chrono::seconds>(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - ts).count();
        auto lt = std::chrono::current_zone()->to_local(ts);

        return std::format("{:%Y-%m-%d %H:%M:%S}.{:06}", lt, ms);
    }

} // namespace utility
