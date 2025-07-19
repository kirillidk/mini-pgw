#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <control_plane.hpp>

class data_plane {
public:
    using Packet = std::vector<uint8_t>;

    explicit data_plane(control_plane &control_plane);

public:
    std::string handle_packet(const Packet &packet);

private:
    control_plane &_control_plane;
};
