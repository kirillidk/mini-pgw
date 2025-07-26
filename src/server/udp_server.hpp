#pragma once

#include <arpa/inet.h>
#include <array>
#include <memory>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>

class config;
class packet_manager;
class logger;

class udp_server_exception : public std::runtime_error {
public:
    explicit udp_server_exception(const std::string &message) :
        std::runtime_error("udp_server_exception: " + message) {}
};

class udp_server {
private:
public:
    udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager,
               std::shared_ptr<logger> logger);
    ~udp_server();

    udp_server(const udp_server &) = delete;
    udp_server &operator=(const udp_server &) = delete;
    udp_server(udp_server &&) = delete;
    udp_server &operator=(udp_server &&) = delete;

    void run();

private:
    static constexpr u_int16_t MAX_EVENTS = 64;
    static constexpr u_int16_t BUFFER_SIZE = 1024;
    static constexpr u_int16_t MAX_BATCH = 10;

private:
    struct pending_request {
        std::span<const uint8_t> data;
        sockaddr_in client_addr;
    };

    struct pending_response {
        std::string data;
        sockaddr_in client_addr;
    };

private:
    void setup(const std::string &ip, int port);

    void read_packets(std::array<uint8_t, BUFFER_SIZE> &buffer);
    void send_pending_responses();
    void process_requests();

    void modify_epoll_events(uint32_t events);

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<packet_manager> _packet_manager;
    std::shared_ptr<logger> _logger;

    int _socket_fd;
    int _epoll_fd;
    std::queue<pending_request> _request_queue;
    std::queue<pending_response> _response_queue;
};
