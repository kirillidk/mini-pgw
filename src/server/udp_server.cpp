#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <span>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <udp_server.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <logger.hpp>
#include <packet_manager.hpp>
#include <thread_pool.hpp>

#include <magic_enum/magic_enum.hpp>

udp_server::udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager,
                       std::shared_ptr<logger> logger, std::shared_ptr<event_bus> event_bus) :
    _config(std::move(config)), _packet_manager(std::move(packet_manager)), _logger(std::move(logger)),
    _event_bus(std::move(event_bus)), _socket_fd(-1), _epoll_fd(-1), _stop_event_fd(-1) {
    auto ip = _config->get_ip().value();
    auto port = _config->get_port().value();

    _logger->debug("Initializing UDP server on " + ip + ":" + std::to_string(port));

    init_setup(ip, port);
    setup_stop_event();
    setup_event_handlers();

    _logger->info("Initialized UDP server on " + ip + ":" + std::to_string(port));
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
    if (_stop_event_fd != -1) {
        close(_stop_event_fd);
        _logger->debug("Stop event fd closed");
    }

    _logger->info("Udp server destroyed");
}

void udp_server::init_setup(const std::string &ip, int port) {
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

    _logger->debug("UDP server init_setup completed successfully");
}

void udp_server::setup_stop_event() {
    _logger->debug("Creating stop event fd");
    _stop_event_fd = eventfd(0, EFD_NONBLOCK);
    if (_stop_event_fd < 0) {
        close(_socket_fd);
        close(_epoll_fd);
        _logger->fatal("Failed to create stop event fd");
        throw udp_server_exception("Failed to create stop event fd");
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _stop_event_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _stop_event_fd, &event) < 0) {
        close(_socket_fd);
        close(_epoll_fd);
        close(_stop_event_fd);
        _logger->fatal("Failed to add stop event fd to epoll");
        throw udp_server_exception("Failed to add stop event fd to epoll");
    }

    _logger->debug("Stop event fd setup completed");
}

void udp_server::setup_event_handlers() {
    _logger->debug("Setting up udp_server event handlers");

    _event_bus->subscribe<events::graceful_shutdown_event>([this]() {
        _logger->debug("Scheduling graceful shutdown for udp server");

        stop();
    });

    _logger->debug("UDP server event handlers setup completed");
}

void udp_server::run() {
    _logger->info("Starting UDP server main loop");
    _running.store(true);

    std::array<epoll_event, MAX_EVENTS> events;
    std::array<uint8_t, BUFFER_SIZE> buffer;

    while (_running.load()) {
        _logger->debug("Waiting for events...");
        int event_count = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            if (errno == EINTR) {
                _logger->debug("epoll_wait interrupted by signal");
                continue;
            }
            _logger->error("epoll_wait error: " + std::string(strerror(errno)));
            break;
        }

        _logger->debug("Received " + std::to_string(event_count) + " events");

        for (int i = 0; i < event_count; ++i) {
            epoll_event &event = events[i];

            if (event.data.fd == _stop_event_fd) {
                _logger->info("Received stop signal");
                _running.store(false);

                uint64_t val;
                if (eventfd_read(_stop_event_fd, &val) < 0) {
                    _logger->error("Failed to read from stop event fd: " + std::string(strerror(errno)));
                }
                break;
            } else if (event.data.fd == _socket_fd) {
                if (event.events & EPOLLIN) {
                    read_packets(buffer);
                }

                if ((event.events & EPOLLOUT) && not _response_queue.empty()) {
                    send_pending_responses();
                }
            }
        }

        process_requests();
    }

    _logger->info("Processing remaining requests before shutdown...");
    while (not _request_queue.empty()) {
        process_requests();
    }

    _logger->info("Sending remaining responses before shutdown...");
    while (not _response_queue.empty()) {
        send_pending_responses();
    }

    _logger->info("UDP server main loop exited gracefully");
}

void udp_server::read_packets(std::array<uint8_t, BUFFER_SIZE> &buffer) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        ssize_t bytes_received =
                recvfrom(_socket_fd, buffer.data(), BUFFER_SIZE, MSG_DONTWAIT, (sockaddr *) &client_addr, &client_len);

        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            _logger->error("recvfrom error: " + std::string(strerror(errno)));
            break;
        }

        if (bytes_received == 0) {
            _logger->debug("Received empty packet, ignoring");
            continue;
        }

        if (static_cast<size_t>(bytes_received) > buffer.size()) {
            _logger->error("Received packet larger than buffer: " + std::to_string(bytes_received));
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        _logger->debug("Received " + std::to_string(bytes_received) + " bytes from " + std::string(client_ip) + ":" +
                       std::to_string(client_port));

        _request_queue.push({std::span(buffer.data(), bytes_received), client_addr});

        _logger->debug("Queued packet (" + std::to_string(bytes_received) + " bytes)");
    }
}

void udp_server::send_pending_responses() {
    int32_t sent_responses = 0;

    while (not _response_queue.empty()) {
        auto &resp = _response_queue.front();

        ssize_t bytes_sent = sendto(_socket_fd, resp.data.data(), resp.data.size(), MSG_DONTWAIT,
                                    (const sockaddr *) &resp.client_addr, sizeof(resp.client_addr));

        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            _logger->error("sendto error: " + std::string(strerror(errno)));
            _response_queue.pop();
            continue;
        }

        _response_queue.pop();
        sent_responses++;
    }

    if (_response_queue.empty()) {
        modify_epoll_events(EPOLLIN);
        _logger->debug("Disabled EPOLLOUT, now only monitoring EPOLLIN on socket");
    }

    if (sent_responses > 0) {
        _logger->debug("Sent " + std::to_string(sent_responses) + " responses");
    }
}

void udp_server::process_requests() {
    int32_t processed_requests = 0;

    while (not _request_queue.empty() && processed_requests < MAX_BATCH) {
        auto req = std::move(_request_queue.front());
        _request_queue.pop();

        auto result = _packet_manager->handle_packet(req.data);

        std::string response =
                result.has_value() ? result.value() : std::format("Error: {}", magic_enum::enum_name(result.error()));

        _response_queue.push({std::move(response), req.client_addr});

        processed_requests++;
    }

    if (processed_requests > 0) {
        _logger->debug("Processed " + std::to_string(processed_requests) + " requests");

        if (not _response_queue.empty()) {
            modify_epoll_events(EPOLLIN | EPOLLOUT);
            _logger->debug("Enabled EPOLLOUT on socket");
        }
    }
}

void udp_server::modify_epoll_events(uint32_t events) {
    epoll_event event;
    event.events = events;
    event.data.fd = _socket_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, _socket_fd, &event);
}

void udp_server::stop() {
    _logger->info("Stopping UDP server...");
    _running.store(false);

    uint64_t val = 1;
    if (eventfd_write(_stop_event_fd, val) < 0) {
        _logger->error("Failed to write to stop event fd: " + std::string(strerror(errno)));
    }
}
