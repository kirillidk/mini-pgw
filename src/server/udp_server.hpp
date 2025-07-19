#pragma once

#include <arpa/inet.h>
#include <string>
#include <vector>

#include <data_plane.hpp>

class udp_server {
private:
    static constexpr int MAX_EVENTS = 64;
    static constexpr int BUFFER_SIZE = 1024;

public:
    udp_server(const std::string &ip, int port, data_plane &dp);
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

    data_plane &_data_plane;
};
