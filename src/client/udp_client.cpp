#include <udp_client.hpp>

#include <config.hpp>
#include <logger.hpp>
#include <utility.hpp>

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <magic_enum/magic_enum.hpp>

udp_client::udp_client(std::shared_ptr<config> config, std::shared_ptr<logger> logger) :
    _config(std::move(config)), _logger(std::move(logger)), _socket_fd(-1), _epoll_fd(-1) {

    auto server_ip = _config->get_ip();
    auto server_port = _config->get_port();

    if (!server_ip.has_value()) {
        _logger->fatal("Server IP not specified in config");
        throw udp_client_exception("Server IP not specified in config");
    }

    if (!server_port.has_value()) {
        _logger->fatal("Server port not specified in config");
        throw udp_client_exception("Server port not specified in config");
    }

    _server_ip = server_ip.value();
    _server_port = server_port.value();

    init_setup(_server_ip, _server_port);

    _logger->info("UDP client initialized for server " + _server_ip + ":" + std::to_string(_server_port));
}

udp_client::~udp_client() {
    cleanup();
    _logger->info("UDP client destroyed");
}

std::expected<std::string, udp_client_error> udp_client::send_imsi(const std::string &imsi) {
    _logger->info("Sending IMSI: " + imsi);

    auto bcd_data = utility::encode_imsi_to_bcd(imsi);
    if (!bcd_data.has_value()) {
        std::string error_msg = "Failed to encode IMSI to BCD: " + std::string(magic_enum::enum_name(bcd_data.error()));
        _logger->error(error_msg);
        return std::unexpected(udp_client_error::invalid_imsi_format);
    }

    _logger->debug("Sending " + std::to_string(bcd_data->size()) + " bytes to server");
    ssize_t bytes_sent = sendto(_socket_fd, bcd_data->data(), bcd_data->size(), MSG_DONTWAIT,
                                (const sockaddr *) &_server_addr, sizeof(_server_addr));

    if (bytes_sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            _logger->error("Failed to send data: " + std::string(strerror(errno)));
            return std::unexpected(udp_client_error::send_failed);
        }
        _logger->warning("Socket not ready for writing, this shouldn't happen with UDP");
        return std::unexpected(udp_client_error::send_failed);
    }

    if (static_cast<size_t>(bytes_sent) != bcd_data->size()) {
        _logger->error("Partial send: sent " + std::to_string(bytes_sent) + " of " + std::to_string(bcd_data->size()) +
                       " bytes");
        return std::unexpected(udp_client_error::send_failed);
    }

    _logger->debug("Successfully sent " + std::to_string(bytes_sent) + " bytes");

    std::array<epoll_event, MAX_EVENTS> events;
    std::array<char, BUFFER_SIZE> buffer{};

    _logger->debug("Waiting for response using epoll...");

    while (true) {
        int event_count = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, TIMEOUT_MS);

        if (event_count < 0) {
            if (errno == EINTR) {
                _logger->debug("epoll_wait interrupted by signal, retrying");
                continue;
            }
            _logger->error("epoll_wait failed: " + std::string(strerror(errno)));
            return std::unexpected(udp_client_error::epoll_wait_failed);
        }

        if (event_count == 0) {
            _logger->error("Receive timeout after " + std::to_string(TIMEOUT_MS) + " ms");
            return std::unexpected(udp_client_error::timeout);
        }

        _logger->debug("Received " + std::to_string(event_count) + " epoll events");

        for (int i = 0; i < event_count; ++i) {
            epoll_event &event = events[i];

            if (event.data.fd == _socket_fd) {
                if (event.events & EPOLLIN) {
                    sockaddr_in from_addr{};
                    socklen_t from_len = sizeof(from_addr);

                    ssize_t bytes_received = recvfrom(_socket_fd, buffer.data(), BUFFER_SIZE - 1, MSG_DONTWAIT,
                                                      (sockaddr *) &from_addr, &from_len);

                    if (bytes_received < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                        _logger->error("Failed to receive response: " + std::string(strerror(errno)));
                        return std::unexpected(udp_client_error::receive_failed);
                    }

                    if (bytes_received == 0) {
                        _logger->warning("Received empty packet, ignoring");
                        continue;
                    }

                    if (from_addr.sin_addr.s_addr != _server_addr.sin_addr.s_addr ||
                        from_addr.sin_port != _server_addr.sin_port) {

                        char from_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, INET_ADDRSTRLEN);
                        _logger->warning("Received response from unexpected source: " + std::string(from_ip) + ":" +
                                         std::to_string(ntohs(from_addr.sin_port)) + ", ignoring");
                        continue;
                    }

                    buffer[bytes_received] = '\0';
                    std::string response(buffer.data(), bytes_received);

                    _logger->info("Received response: " + response + " (" + std::to_string(bytes_received) + " bytes)");
                    return response;
                }

                if (event.events & EPOLLERR) {
                    _logger->error("Socket error event received");
                    return std::unexpected(udp_client_error::receive_failed);
                }

                if (event.events & EPOLLHUP) {
                    _logger->error("Socket hang up event received");
                    return std::unexpected(udp_client_error::receive_failed);
                }
            }
        }
    }
}

void udp_client::init_setup(const std::string &ip, uint32_t port) {
    _logger->debug("Creating UDP socket");
    _socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket_fd < 0) {
        _logger->fatal("Failed to create socket: " + std::string(strerror(errno)));
        throw udp_client_exception("Failed to create socket");
    }

    _server_addr = {};
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &_server_addr.sin_addr) != 1) {
        close(_socket_fd);
        _logger->fatal("Invalid IP address: " + ip);
        throw udp_client_exception("Invalid IP address");
    }

    _logger->debug("Setting socket to non-blocking mode");
    int flags = fcntl(_socket_fd, F_GETFL, 0);
    if (flags < 0) {
        close(_socket_fd);
        _logger->fatal("fcntl F_GETFL failed");
        throw udp_client_exception("fcntl F_GETFL failed");
    }
    if (fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(_socket_fd);
        _logger->fatal("fcntl F_SETFL O_NONBLOCK failed");
        throw udp_client_exception("fcntl F_SETFL O_NONBLOCK failed");
    }

    _logger->debug("Creating epoll instance");
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        close(_socket_fd);
        _logger->fatal("Failed to create epoll");
        throw udp_client_exception("Failed to create epoll");
    }

    epoll_event event;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    event.data.fd = _socket_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _socket_fd, &event) < 0) {
        close(_socket_fd);
        close(_epoll_fd);
        _logger->fatal("Failed to add socket to epoll");
        throw udp_client_exception("Failed to add socket to epoll");
    }

    _logger->debug("UDP client init_setup completed successfully");
}

void udp_client::cleanup() {
    if (_socket_fd != -1) {
        close(_socket_fd);
        _socket_fd = -1;
        _logger->debug("Socket closed");
    }

    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
        _logger->debug("Epoll fd closed");
    }
}
