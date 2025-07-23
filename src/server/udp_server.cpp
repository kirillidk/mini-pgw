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
#include <magic_enum/magic_enum.hpp>
#include <packet_manager.hpp>

udp_server::udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager) :
    _socket_fd(-1), _epoll_fd(-1), _config(std::move(config)), _packet_manager(std::move(packet_manager)) {
    setup(_config->get_ip().value(), _config->get_port().value());
}

udp_server::~udp_server() {
    if (_socket_fd != -1) {
        close(_socket_fd);
    }
    if (_epoll_fd != -1) {
        close(_epoll_fd);
    }
}

void udp_server::setup(const std::string &ip, int port) {
    _socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket_fd < 0) {
        throw udp_server_exception("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        close(_socket_fd);
        throw udp_server_exception("Invalid IP address");
    }

    if (bind(_socket_fd, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(_socket_fd);
        throw udp_server_exception("Failed to bind socket");
    }

    int flags = fcntl(_socket_fd, F_GETFL, 0);
    if (flags < 0) {
        close(_socket_fd);
        throw udp_server_exception("fcntl F_GETFL failed");
    }
    if (fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(_socket_fd);
        throw udp_server_exception("fcntl F_SETFL O_NONBLOCK failed");
    }

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        close(_socket_fd);
        throw udp_server_exception("Failed to create epoll");
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _socket_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _socket_fd, &event) < 0) {
        close(_socket_fd);
        close(_epoll_fd);
        throw udp_server_exception("Failed to add socket to epoll");
    }
}


void udp_server::run() {
    std::vector<epoll_event> events(MAX_EVENTS);
    std::vector<uint8_t> buffer(BUFFER_SIZE);

    while (true) {
        int event_count = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            std::cerr << "epoll_wait error" << std::endl;
            break;
        }

        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == _socket_fd) {
                handle_packet(buffer);
            }
        }
    }
}

void udp_server::handle_packet(std::vector<uint8_t> &buffer) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        ssize_t bytes_received =
                recvfrom(_socket_fd, buffer.data(), BUFFER_SIZE, MSG_DONTWAIT, (sockaddr *) &client_addr, &client_len);

        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            break;
        }

        buffer.resize(bytes_received);

        std::expected<std::string, packet_manager_error> packet_result = _packet_manager->handle_packet(buffer);

        std::string response = packet_result.has_value()
                                       ? packet_result.value()
                                       : std::format("Error: {}", magic_enum::enum_name(packet_result.error()));

        send_response(response, client_addr);
    }
}

void udp_server::send_response(const std::string &response, const sockaddr_in &client_addr) {
    sendto(_socket_fd, response.data(), response.size(), 0, (const sockaddr *) &client_addr, sizeof(client_addr));
}
