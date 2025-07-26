#include <regex>

#include <http_server.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <logger.hpp>
#include <session_manager.hpp>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>


http_server::http_server(std::shared_ptr<config> config, std::shared_ptr<session_manager> session_manager,
                         std::shared_ptr<logger> logger, std::shared_ptr<event_bus> event_bus) :
    _config(std::move(config)), _session_manager(std::move(session_manager)), _logger(std::move(logger)),
    _event_bus(std::move(event_bus)), _ioc(1), _acceptor(_ioc) {
    init_setup();
    setup_event_handlers();
}

void http_server::init_setup() {
    auto ip = _config->get_ip().value();
    auto port = _config->get_http_port().value();

    _logger->info("Initializing HTTP server on " + ip + ":" + std::to_string(port));

    beast::error_code ec;

    auto const address = net::ip::make_address(ip, ec);
    if (ec) {
        _logger->fatal("Invalid HTTP server IP address: " + ip);
        throw http_server_exception("Invalid IP address: " + ip);
    }

    _acceptor.open(address.is_v6() ? tcp::v6() : tcp::v4(), ec);
    if (ec) {
        _logger->fatal("Failed to open HTTP acceptor");
        throw http_server_exception("Failed to open acceptor");
    }

    _acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        _logger->fatal("Failed to set HTTP acceptor reuse_address option");
        throw http_server_exception("Failed to set reuse_address");
    }

    _acceptor.bind({address, boost::asio::ip::port_type(port)}, ec);
    if (ec) {
        _logger->fatal("Failed to bind HTTP acceptor to " + ip + ":" + std::to_string(port));
        throw http_server_exception("Failed to bind acceptor");
    }

    _acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        _logger->fatal("Failed to listen on HTTP acceptor");
        throw http_server_exception("Failed to listen");
    }

    _logger->info("HTTP server setup completed successfully");
}


void http_server::setup_event_handlers() {
    _logger->info("Setting up http server event handlers");

    _event_bus->subscribe<events::graceful_shutdown_event>([this]() { this->stop(); });

    _logger->info("Htpp server setup of event handlers is completed");
}

http_server::~http_server() {
    _logger->info("Shutting down HTTP server");
    stop();
    _logger->debug("Http server is destroyed");
}

void http_server::run() {
    _logger->info("Starting HTTP server");
    do_accept();
    _ioc.run();
    _logger->info("HTTP server stopped");
}

void http_server::stop() {
    _logger->info("Stopping HTTP server");
    _stop_requested = true;
    _ioc.stop();
}

void http_server::do_accept() {
    _acceptor.async_accept(net::make_strand(_ioc),
                           beast::bind_front_handler(&http_server::on_accept, shared_from_this()));
}

void http_server::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        if (ec != net::error::operation_aborted) {
            _logger->error("HTTP accept error: " + ec.message());
        }
        return;
    }

    _logger->debug("New HTTP connection accepted");
    std::make_shared<session>(std::move(socket), shared_from_this())->run();

    if (!_stop_requested) {
        do_accept();
    }
}

http_server::session::session(tcp::socket &&socket, std::shared_ptr<http_server> server) :
    _stream(std::move(socket)), _server(std::move(server)) {}

void http_server::session::run() {
    net::dispatch(_stream.get_executor(), beast::bind_front_handler(&session::do_read, shared_from_this()));
}

void http_server::session::do_read() {
    _req = {};

    _stream.expires_after(std::chrono::seconds(30));

    http::async_read(_stream, _buffer, _req, beast::bind_front_handler(&session::on_read, shared_from_this()));
}

void http_server::session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        return;
    }

    if (ec) {
        _server->_logger->error("HTTP read error: " + ec.message());
        return;
    }

    _server->_logger->debug("HTTP request: " + std::string(_req.method_string()) + " " + std::string(_req.target()));

    auto response = handle_request(std::move(_req));

    bool keep_alive = response.keep_alive();

    auto sp = std::make_shared<http::response<http::string_body>>(std::move(response));

    http::async_write(_stream, *sp,
                      [self = shared_from_this(), sp, keep_alive](beast::error_code ec, std::size_t bytes_transferred) {
                          self->on_write(ec, bytes_transferred, !keep_alive);
                      });
}

void http_server::session::on_write(beast::error_code ec, std::size_t bytes_transferred, bool close) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        _server->_logger->error("HTTP write error: " + ec.message());
        return;
    }

    if (close) {
        _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    do_read();
}

http::response<http::string_body> http_server::session::handle_request(http::request<http::string_body> &&req) {
    if (req.method() != http::verb::get) {
        return bad_request("Unknown HTTP-method");
    }

    std::string target = req.target();

    if (target == "/stop") {
        return handle_stop();
    }

    if (target.starts_with("/check_subscriber")) {
        std::regex imsi_regex(R"(/check_subscriber\?imsi=(\d{6,15}))");
        std::smatch match;

        if (std::regex_match(target, match, imsi_regex) && match.size() == 2) {
            std::string imsi = match[1].str();
            return handle_check_subscriber(imsi);
        } else {
            return bad_request("Invalid IMSI format. Expected: /check_subscriber?imsi=<6-15 digits>");
        }
    }

    return not_found(target);
}

http::response<http::string_body> http_server::session::handle_check_subscriber(const std::string &imsi) {
    _server->_logger->debug("Checking subscriber status for IMSI: " + imsi);

    bool is_active = _server->_session_manager->has_active_session(imsi);

    http::response<http::string_body> res{http::status::ok, _req.version()};
    res.set(http::field::server, "SessionServer/1.0");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(_req.keep_alive());

    std::string response_body = is_active ? "active" : "not active";
    res.body() = response_body;
    res.prepare_payload();

    _server->_logger->info("Subscriber " + imsi + " status: " + response_body);

    return res;
}

http::response<http::string_body> http_server::session::handle_stop() {
    _server->_logger->info("Received stop request via HTTP API");

    http::response<http::string_body> res{http::status::ok, _req.version()};
    res.set(http::field::server, "SessionServer/1.0");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(false);
    res.body() = "Server shutdown initiated";
    res.prepare_payload();

    _server->_event_bus->publish<events::graceful_shutdown_event>();

    return res;
}

http::response<http::string_body> http_server::session::bad_request(const std::string &why) {
    http::response<http::string_body> res{http::status::bad_request, _req.version()};
    res.set(http::field::server, "SessionServer/1.0");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(_req.keep_alive());
    res.body() = why;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> http_server::session::not_found(const std::string &target) {
    http::response<http::string_body> res{http::status::not_found, _req.version()};
    res.set(http::field::server, "SessionServer/1.0");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(_req.keep_alive());
    res.body() = "The resource '" + target + "' was not found.";
    res.prepare_payload();
    return res;
}

http::response<http::string_body> http_server::session::server_error(const std::string &what) {
    http::response<http::string_body> res{http::status::internal_server_error, _req.version()};
    res.set(http::field::server, "SessionServer/1.0");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(_req.keep_alive());
    res.body() = "An error occurred: '" + what + "'";
    res.prepare_payload();
    return res;
}
