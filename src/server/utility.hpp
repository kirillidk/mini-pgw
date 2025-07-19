#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace utility {
    [[nodiscard]] std::string parse_imsi_from_bcd(const std::vector<uint8_t> &packet);
} // namespace utility
