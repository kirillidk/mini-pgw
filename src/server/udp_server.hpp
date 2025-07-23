#pragma once

#include <arpa/inet.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class config;
class packet_manager;

class udp_server_exception : public std::runtime_error {
public:
    explicit udp_server_exception(const std::string &message) :
        std::runtime_error("udp_server_exception: " + message) {}
};

class udp_server {
private:
    static constexpr int MAX_EVENTS = 64;
    static constexpr int BUFFER_SIZE = 1024;

public:
    udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager);
    ~udp_server();

    udp_server(const udp_server &) = delete;
    udp_server &operator=(const udp_server &) = delete;
    udp_server(udp_server &&) = delete;
    udp_server &operator=(udp_server &&) = delete;

    void run();

private:
    void setup(const std::string &ip, int port);

    void handle_packet(std::vector<uint8_t> &buffer);
    void send_response(const std::string &response, const sockaddr_in &client_addr);

private:
    int _socket_fd;
    int _epoll_fd;

    std::shared_ptr<config> _config;
    std::shared_ptr<packet_manager> _packet_manager;
};
