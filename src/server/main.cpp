#include <iostream>

#include <config.hpp>
#include <control_plane.hpp>
#include <data_plane.hpp>
#include <udp_server.hpp>


int main() {
    try {
        config cfg("config.json");

        control_plane cp(cfg);
        data_plane dp(cp);

        udp_server server(dp);

        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
