#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <udp_server.hpp>

#include <config.hpp>
#include <logger.hpp>
#include <magic_enum/magic_enum.hpp>
#include <packet_manager.hpp>

udp_server::udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager,
                       std::shared_ptr<logger> logger) :
    _socket_fd(-1), _epoll_fd(-1), _config(std::move(config)), _packet_manager(std::move(packet_manager)),
    _logger(std::move(logger)) {

    auto ip = _config->get_ip().value();
    auto port = _config->get_port().value();

    _logger->info("Initializing UDP server on " + ip + ":" + std::to_string(port));
    setup(ip, port);
}

udp_server::~udp_server() {
    _logger->info("Shutting down UDP server");

    if (_socket_fd != -1) {
        close(_socket_fd);
        _logger->debug("Socket closed");
    }
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _logger->debug("Epoll fd closed");
    }
}

void udp_server::setup(const std::string &ip, int port) {
    _logger->debug("Creating UDP socket");
    _socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket_fd < 0) {
        _logger->fatal("Failed to create socket");
        throw udp_server_exception("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        close(_socket_fd);
        _logger->fatal("Invalid IP address: " + ip);
        throw udp_server_exception("Invalid IP address");
    }

    _logger->debug("Binding socket to address");
    if (bind(_socket_fd, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(_socket_fd);
        _logger->fatal("Failed to bind socket to " + ip + ":" + std::to_string(port));
        throw udp_server_exception("Failed to bind socket");
    }

    _logger->debug("Setting socket to non-blocking mode");
    int flags = fcntl(_socket_fd, F_GETFL, 0);
    if (flags < 0) {
        close(_socket_fd);
        _logger->fatal("fcntl F_GETFL failed");
        throw udp_server_exception("fcntl F_GETFL failed");
    }
    if (fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(_socket_fd);
        _logger->fatal("fcntl F_SETFL O_NONBLOCK failed");
        throw udp_server_exception("fcntl F_SETFL O_NONBLOCK failed");
    }

    _logger->debug("Creating epoll instance");
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        close(_socket_fd);
        _logger->fatal("Failed to create epoll");
        throw udp_server_exception("Failed to create epoll");
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _socket_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _socket_fd, &event) < 0) {
        close(_socket_fd);
        close(_epoll_fd);
        _logger->fatal("Failed to add socket to epoll");
        throw udp_server_exception("Failed to add socket to epoll");
    }

    _logger->info("UDP server setup completed successfully");
}

void udp_server::run() {
    _logger->info("Starting UDP server main loop");

    std::vector<epoll_event> events(MAX_EVENTS);
    std::vector<uint8_t> buffer(BUFFER_SIZE);

    while (true) {
        _logger->debug("Waiting for events...");
        int event_count = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            _logger->error("epoll_wait error: " + std::string(strerror(errno)));
            break;
        }

        _logger->debug("Received " + std::to_string(event_count) + " events");

        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == _socket_fd) {
                handle_packet(buffer);
            }
        }
    }

    _logger->info("UDP server main loop exited");
}

void udp_server::handle_packet(std::vector<uint8_t> &buffer) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        ssize_t bytes_received =
                recvfrom(_socket_fd, buffer.data(), BUFFER_SIZE, MSG_DONTWAIT, (sockaddr *) &client_addr, &client_len);

        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more data available
            }
            _logger->error("recvfrom error: " + std::string(strerror(errno)));
            break;
        }

        buffer.resize(bytes_received);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        _logger->debug("Received " + std::to_string(bytes_received) + " bytes from " + std::string(client_ip) + ":" +
                       std::to_string(client_port));

        std::expected<std::string, packet_manager_error> packet_result = _packet_manager->handle_packet(buffer);

        std::string response = packet_result.has_value()
                                       ? packet_result.value()
                                       : std::format("Error: {}", magic_enum::enum_name(packet_result.error()));

        _logger->debug("Sending response: " + response + " to " + std::string(client_ip) + ":" +
                       std::to_string(client_port));

        send_response(response, client_addr);
    }
}

void udp_server::send_response(const std::string &response, const sockaddr_in &client_addr) {
    ssize_t bytes_sent = sendto(_socket_fd, response.data(), response.size(), 0, (const sockaddr *) &client_addr,
                                sizeof(client_addr));

    if (bytes_sent < 0) {
        _logger->error("Failed to send response: " + std::string(strerror(errno)));
    } else {
        _logger->debug("Response sent successfully (" + std::to_string(bytes_sent) + " bytes)");
    }
}
