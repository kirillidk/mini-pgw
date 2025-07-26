#include <http_server.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <logger.hpp>
#include <session_manager.hpp>

#include <regex>

http_server::http_server(std::shared_ptr<config> config, std::shared_ptr<session_manager> session_manager,
                         std::shared_ptr<event_bus> event_bus, std::shared_ptr<logger> logger) :
    _config(std::move(config)), _session_manager(std::move(session_manager)), _event_bus(std::move(event_bus)),
    _logger(std::move(logger)) {

    auto ip = _config->get_ip();
    auto http_port = _config->get_http_port();

    if (!ip.has_value()) {
        _logger->fatal("HTTP server IP not specified in config");
        throw http_server_exception("HTTP server IP not specified in config");
    }

    if (!http_port.has_value()) {
        _logger->fatal("HTTP server port not specified in config");
        throw http_server_exception("HTTP server port not specified in config");
    }

    _ip = ip.value();
    _port = http_port.value();

    _logger->info("Initializing HTTP server on " + _ip + ":" + std::to_string(_port));

    _server = std::make_unique<httplib::Server>();
    setup_routes();
}

http_server::~http_server() {
    stop();
    _logger->debug("HTTP server is destroyed");
}

void http_server::setup_routes() {
    _logger->info("Setting up HTTP server routes");

    _server->Get("/check_subscriber",
                 [this](const httplib::Request &req, httplib::Response &res) { handle_check_subscriber(req, res); });

    _server->Post("/stop", [this](const httplib::Request &req, httplib::Response &res) { handle_stop(req, res); });

    _server->set_error_handler([this](const httplib::Request &req, httplib::Response &res) {
        _logger->warning("Unknown HTTP endpoint: " + req.method + " " + req.path);
        res.status = 404;
        res.set_content("Not Found", "text/plain");
    });

    _logger->info("HTTP server routes setup completed");
}

void http_server::handle_check_subscriber(const httplib::Request &req, httplib::Response &res) {
    _logger->debug("Received check_subscriber request from " + req.remote_addr);

    if (!req.has_param("imsi")) {
        _logger->warning("Missing 'imsi' parameter in check_subscriber request");
        res.status = 400;
        res.set_content("Bad Request: 'imsi' parameter is required", "text/plain");
        return;
    }

    std::string imsi = req.get_param_value("imsi");

    if (imsi.empty()) {
        _logger->warning("Empty IMSI parameter in check_subscriber request");
        res.status = 400;
        res.set_content("Bad Request: 'imsi' parameter cannot be empty", "text/plain");
        return;
    }

    try {
        if (imsi.length() < 6 || imsi.length() > 15) {
            _logger->warning("Invalid IMSI length: " + imsi);
            res.status = 400;
            res.set_content("Bad Request: IMSI length must be between 6 and 15 digits", "text/plain");
            return;
        }

        if (!std::regex_match(imsi, std::regex("^[0-9]+$"))) {
            _logger->warning("Invalid IMSI format: " + imsi);
            res.status = 400;
            res.set_content("Bad Request: IMSI must contain only digits", "text/plain");
            return;
        }

        _logger->debug("Checking session status for IMSI: " + imsi);

        bool is_active = _session_manager->has_active_session(imsi);
        std::string response = is_active ? "active" : "not active";

        _logger->info("Session status for IMSI " + imsi + ": " + response);

        res.status = 200;
        res.set_content(response, "text/plain");

    } catch (const std::exception &e) {
        _logger->error("Error processing check_subscriber request: " + std::string(e.what()));
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    }
}

void http_server::handle_stop(const httplib::Request &req, httplib::Response &res) {
    _logger->info("Received graceful shutdown request from " + req.remote_addr);

    try {
        _logger->info("Initiating graceful shutdown via HTTP API");

        _event_bus->publish<events::graceful_shutdown_event>();

        res.status = 200;
        res.set_content("Graceful shutdown initiated", "text/plain");

        _logger->info("Graceful shutdown request processed successfully");

    } catch (const std::exception &e) {
        _logger->error("Error processing stop request: " + std::string(e.what()));
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    }
}

void http_server::start() {
    if (_running.load()) {
        _logger->warning("HTTP server is already running");
        return;
    }

    _logger->info("Starting HTTP server...");

    _running.store(true);

    _server_thread = std::make_unique<std::thread>([this]() {
        _logger->info("HTTP server thread started");

        if (!_server->listen(_ip, _port)) {
            _logger->fatal("Failed to start HTTP server on " + _ip + ":" + std::to_string(_port));
            _running.store(false);
            throw http_server_exception("Failed to start HTTP server");
        }

        _logger->info("HTTP server stopped");
    });

    if (!_running.load()) {
        if (_server_thread && _server_thread->joinable()) {
            _server_thread->join();
        }
        throw http_server_exception("HTTP server failed to start");
    }

    _logger->info("HTTP server started successfully on " + _ip + ":" + std::to_string(_port));
}

void http_server::stop() {
    if (!_running.load()) {
        return;
    }

    _logger->info("Stopping HTTP server...");

    _running.store(false);

    if (_server) {
        _server->stop();
    }

    if (_server_thread && _server_thread->joinable()) {
        _server_thread->join();
    }

    _logger->info("HTTP server stopped successfully");
}
