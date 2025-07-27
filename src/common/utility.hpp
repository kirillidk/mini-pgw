#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace utility {
    enum class decode_error {
        packet_too_short,
        invalid_imsi_type,
        packet_size_mismatch,
        invalid_bcd_digit,
        invalid_imsi_length
    };

    enum class encode_error { invalid_imsi_format };

    [[nodiscard]] std::expected<std::string, decode_error> decode_imsi_from_bcd(std::span<const uint8_t> packet);
    [[nodiscard]] std::expected<std::vector<uint8_t>, encode_error> encode_imsi_to_bcd(const std::string &imsi);
    [[nodiscard]] bool is_valid_imsi(const std::string &imsi);
    [[nodiscard]] std::string get_current_timestamp();
} // namespace utility
