#include <iostream>
#include <memory>

#include <config.hpp>
#include <logger.hpp>
#include <udp_client.hpp>
#include <utility.hpp>

#include <magic_enum/magic_enum.hpp>

#include <boost/di.hpp>
namespace di = boost::di;

void print_usage(const char *program_name) {
    std::cout << "Usage: " << program_name << " <IMSI> [config_file]\n";
    std::cout << "  IMSI: International Mobile Subscriber Identity (6-15 digits)\n";
    std::cout << "  config_file: Path to JSON configuration file (default: client_config.json)\n";
    std::cout << "\nExample: " << program_name << " 001010123456789 client_config.json\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string imsi = argv[1];
    std::string config_file = (argc == 3) ? argv[2] : "client_config.json";

    if (not utility::is_valid_imsi(imsi)) {
        std::cerr << "Error: Invalid IMSI format. IMSI must be 6-15 digits.\n";
        std::cerr << "Provided IMSI: " << imsi << "\n";
        return 2;
    }

    try {
        auto injector = di::make_injector(di::bind<std::filesystem::path>.to(std::filesystem::path{config_file}));

        auto logger_ = injector.create<std::shared_ptr<logger>>();
        auto udp_client_ = injector.create<std::shared_ptr<udp_client>>();

        auto result = udp_client_->send_imsi(imsi);

        if (result.has_value()) {
            std::string response = result.value();
            std::cout << "Server response: " << response << std::endl;

            logger_->info("Request completed successfully");
            logger_->info("Server response: " + response);

            if (response == "created") {
                logger_->info("Session created successfully");
                return 0;
            } else if (response == "rejected") {
                logger_->info("Session was rejected");
                return 3;
            } else {
                logger_->warning("Unexpected server response: " + response);
                return 4;
            }
        } else {
            udp_client_error error = result.error();
            std::string error_str = std::string(magic_enum::enum_name(error));

            std::cerr << "Error: " << error_str << std::endl;

            logger_->error("Request failed: " + error_str);
            return 5;
        }
    }

    catch (const config_exception &e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 6;
    } catch (const logger_exception &e) {
        std::cerr << "Logger error: " << e.what() << std::endl;
        return 7;
    } catch (const udp_client_exception &e) {
        std::cerr << "UDP client error: " << e.what() << std::endl;
        return 8;
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 9;
    }
}
