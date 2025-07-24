#include <iostream>

#include <cdr_writer.hpp>
#include <config.hpp>
#include <event_bus.hpp>
#include <packet_manager.hpp>
#include <session_manager.hpp>
#include <thread_pool.hpp>
#include <udp_server.hpp>

#include <boost/di.hpp>
namespace di = boost::di;

int main() {
    try {
        auto injector = di::make_injector(di::bind<std::filesystem::path>.to(std::filesystem::path{"config.json"}),
                                          di::bind<std::size_t>.to(size_t(std::thread::hardware_concurrency())));

        auto writer = injector.create<std::shared_ptr<cdr_writer>>();

        auto server = injector.create<std::shared_ptr<udp_server>>();
        server->run();

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
