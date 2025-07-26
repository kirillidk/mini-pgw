#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class config;
class session_manager;
class logger;

class http_server_exception : public std::runtime_error {
public:
    explicit http_server_exception(const std::string &message) :
        std::runtime_error("http_server_exception: " + message) {}
};

class http_server : public std::enable_shared_from_this<http_server> {
public:
    http_server(std::shared_ptr<config> config, std::shared_ptr<session_manager> session_manager,
                std::shared_ptr<logger> logger);
    ~http_server();

    http_server(const http_server &) = delete;
    http_server &operator=(const http_server &) = delete;
    http_server(http_server &&) = delete;
    http_server &operator=(http_server &&) = delete;

    void run();
    void stop();

private:
    class session : public std::enable_shared_from_this<session> {
    public:
        session(tcp::socket &&socket, std::shared_ptr<http_server> server);

        void run();

    private:
        void do_read();
        void on_read(beast::error_code ec, std::size_t bytes_transferred);
        void on_write(beast::error_code ec, std::size_t bytes_transferred, bool close);

        [[nodiscard]] http::response<http::string_body> handle_request(http::request<http::string_body> &&req);

        [[nodiscard]] http::response<http::string_body> handle_check_subscriber(const std::string &imsi);
        [[nodiscard]] http::response<http::string_body> handle_stop();

        [[nodiscard]] http::response<http::string_body> bad_request(const std::string &why);
        [[nodiscard]] http::response<http::string_body> not_found(const std::string &target);
        [[nodiscard]] http::response<http::string_body> server_error(const std::string &what);

    private:
        beast::tcp_stream _stream;
        beast::flat_buffer _buffer;
        http::request<http::string_body> _req;
        std::shared_ptr<http_server> _server;
    };

    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);

private:
    std::shared_ptr<config> _config;
    std::shared_ptr<session_manager> _session_manager;
    std::shared_ptr<logger> _logger;

    net::io_context _ioc;
    tcp::acceptor _acceptor;
    std::atomic<bool> _stop_requested{false};
};
