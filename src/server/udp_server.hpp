#pragma once

#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <memory>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>

class config;
class packet_manager;
class logger;
class event_bus;
class thread_pool;

class udp_server_exception : public std::runtime_error {
public:
    explicit udp_server_exception(const std::string &message) :
        std::runtime_error("udp_server_exception: " + message) {}
};

class udp_server {
private:
public:
    udp_server(std::shared_ptr<config> config, std::shared_ptr<packet_manager> packet_manager,
               std::shared_ptr<logger> logger, std::shared_ptr<event_bus> event_bus,
               std::shared_ptr<thread_pool> thread_pool);
    ~udp_server();

    udp_server(const udp_server &) = delete;
    udp_server &operator=(const udp_server &) = delete;
    udp_server(udp_server &&) = delete;
    udp_server &operator=(udp_server &&) = delete;

    void run();
    void stop();

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
    void setup_stop_event();

    void read_packets(std::array<uint8_t, BUFFER_SIZE> &buffer);
    void send_pending_responses();
    void process_requests();

    void modify_epoll_events(uint32_t events);

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<packet_manager> _packet_manager;
    std::shared_ptr<logger> _logger;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<thread_pool> _thread_pool;

    int _socket_fd;
    int _epoll_fd;
    int _stop_event_fd;

    std::queue<pending_request> _request_queue;
    std::queue<pending_response> _response_queue;

    std::atomic<bool> _running{false};
};
