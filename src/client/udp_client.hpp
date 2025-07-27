#pragma once

#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <expected>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <vector>

class config;
class logger;

enum class udp_client_error {
    socket_creation_failed,
    invalid_address,
    send_failed,
    receive_failed,
    timeout,
    invalid_imsi_format,
    epoll_creation_failed,
    epoll_ctl_failed,
    epoll_wait_failed
};

class udp_client_exception : public std::runtime_error {
public:
    explicit udp_client_exception(const std::string &message) :
        std::runtime_error("udp_client_exception: " + message) {}
};

class udp_client {
public:
    explicit udp_client(std::shared_ptr<config> config, std::shared_ptr<logger> logger);
    ~udp_client();

    udp_client(const udp_client &) = delete;
    udp_client &operator=(const udp_client &) = delete;
    udp_client(udp_client &&) = delete;
    udp_client &operator=(udp_client &&) = delete;

    [[nodiscard]] std::expected<std::string, udp_client_error> send_imsi(const std::string &imsi);

private:
    void init_setup(const std::string &ip, uint32_t port);
    void cleanup();

private:
    static constexpr uint32_t TIMEOUT_MS = 5000;
    static constexpr uint32_t BUFFER_SIZE = 1024;
    static constexpr uint32_t MAX_EVENTS = 10;

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<logger> _logger;

    int _socket_fd;
    int _epoll_fd;

    std::string _server_ip;
    uint32_t _server_port;
    sockaddr_in _server_addr;
};
