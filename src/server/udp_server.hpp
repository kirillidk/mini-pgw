#pragma once

#include <arpa/inet.h>
#include <string>

class udp_server {
public:
    udp_server(const std::string &ip, int port);
    ~udp_server();

    udp_server(const udp_server &) = delete;
    udp_server &operator=(const udp_server &) = delete;

    udp_server(udp_server &&other) noexcept;
    udp_server &operator=(udp_server &&other) noexcept;

    void run();

private:
    void setup(const std::string &ip, int port);
    void handlePacket(char *buffer);

private:
    int _socket_fd;
    int _epoll_fd;

    static constexpr int MAX_EVENTS = 64;
    static constexpr int BUFFER_SIZE = 1024;
};
