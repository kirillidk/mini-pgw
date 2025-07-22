#include <iostream>

#include <config.hpp>
#include <control_plane.hpp>
#include <data_plane.hpp>
#include <thread_pool.hpp>
#include <udp_server.hpp>

int main() {
    try {
        config cfg("config.json");

        thread_pool tp{};

        control_plane cp(cfg, tp);
        data_plane dp(cp);

        udp_server server(dp);
        server.run();
    } catch (const config_exception &e) {
        std::cerr << e.what() << '\n';
        return 2;
    } catch (const udp_server_exception &e) {
        std::cerr << e.what() << "\n";
        return 3;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
