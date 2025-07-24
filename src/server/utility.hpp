#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace utility {
    enum class parse_error {
        packet_too_short,
        invalid_imsi_type,
        packet_size_mismatch,
        invalid_bcd_digit,
        invalid_imsi_length
    };

    [[nodiscard]] std::expected<std::string, parse_error> parse_imsi_from_bcd(const std::vector<uint8_t> &packet);
    [[nodiscard]] std::string get_current_timestamp();
} // namespace utility
