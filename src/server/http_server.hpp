#pragma once

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <httplib.h>

class config;
class session_manager;
class event_bus;
class logger;

class http_server_exception : public std::runtime_error {
public:
    explicit http_server_exception(const std::string &message) :
        std::runtime_error("http_server_exception: " + message) {}
};

class http_server {
public:
    explicit http_server(std::shared_ptr<config> config, std::shared_ptr<session_manager> session_manager,
                         std::shared_ptr<event_bus> event_bus, std::shared_ptr<logger> logger);
    ~http_server();

    http_server(const http_server &) = delete;
    http_server &operator=(const http_server &) = delete;
    http_server(http_server &&) = delete;
    http_server &operator=(http_server &&) = delete;

    void start();
    void stop();

private:
    void setup_routes();

    void handle_check_subscriber(const httplib::Request &req, httplib::Response &res);
    void handle_stop(const httplib::Request &req, httplib::Response &res);

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<session_manager> _session_manager;
    std::shared_ptr<event_bus> _event_bus;
    std::shared_ptr<logger> _logger;

    std::unique_ptr<httplib::Server> _server;
    std::unique_ptr<std::thread> _server_thread;
    std::atomic<bool> _running{false};

    std::string _ip;
    uint32_t _port;
};
