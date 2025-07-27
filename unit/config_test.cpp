#include <config.hpp>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override { test_config_path = std::filesystem::temp_directory_path() / "test_config.json"; }

    void TearDown() override {
        if (std::filesystem::exists(test_config_path)) {
            std::filesystem::remove(test_config_path);
        }
    }

    void WriteConfigFile(const std::string &content) {
        std::ofstream file(test_config_path);
        file << content;
        file.close();
    }

    std::filesystem::path test_config_path;
};

TEST_F(ConfigTest, ValidConfig) {
    WriteConfigFile(R"({
        "server_ip": "127.0.0.1",
        "server_port": 8080,
        "http_port": 8081,
        "session_timeout_sec": 300,
        "cdr_file": "/tmp/cdr.log",
        "graceful_shutdown_rate": 10,
        "log_file": "/tmp/server.log",
        "log_level": "info",
        "blacklist": ["123456", "789012"]
    })");

    config cfg(test_config_path);

    EXPECT_EQ(cfg.get_ip().value(), "127.0.0.1");
    EXPECT_EQ(cfg.get_port().value(), 8080);
    EXPECT_EQ(cfg.get_http_port().value(), 8081);
    EXPECT_EQ(cfg.get_session_timeout_sec().value(), 300);
    EXPECT_EQ(cfg.get_cdr_file().value(), "/tmp/cdr.log");
    EXPECT_EQ(cfg.get_graceful_shutdown_rate().value(), 10);
    EXPECT_EQ(cfg.get_log_file().value(), "/tmp/server.log");
    EXPECT_EQ(cfg.get_log_level().value(), "info");

    auto blacklist = cfg.get_blacklist().value();
    EXPECT_EQ(blacklist.size(), 2);
    EXPECT_TRUE(blacklist.contains("123456"));
    EXPECT_TRUE(blacklist.contains("789012"));
}

TEST_F(ConfigTest, PartialConfig) {
    WriteConfigFile(R"({
        "server_ip": "192.168.1.1",
        "server_port": 9090
    })");

    config cfg(test_config_path);

    EXPECT_TRUE(cfg.get_ip().has_value());
    EXPECT_TRUE(cfg.get_port().has_value());
    EXPECT_FALSE(cfg.get_http_port().has_value());
    EXPECT_FALSE(cfg.get_session_timeout_sec().has_value());
}

TEST_F(ConfigTest, NullValues) {
    WriteConfigFile(R"({
        "server_ip": null,
        "server_port": 8080,
        "blacklist": null
    })");

    config cfg(test_config_path);

    EXPECT_FALSE(cfg.get_ip().has_value());
    EXPECT_TRUE(cfg.get_port().has_value());
    EXPECT_FALSE(cfg.get_blacklist().has_value());
}

TEST_F(ConfigTest, EmptyBlacklist) {
    WriteConfigFile(R"({
        "server_ip": "127.0.0.1",
        "blacklist": []
    })");

    config cfg(test_config_path);

    auto blacklist = cfg.get_blacklist().value();
    EXPECT_TRUE(blacklist.empty());
}

TEST_F(ConfigTest, FileNotFound) { EXPECT_THROW(config cfg("/nonexistent/path.json"), config_exception); }

TEST_F(ConfigTest, InvalidJson) {
    WriteConfigFile("{ invalid json }");

    EXPECT_THROW(config cfg(test_config_path), config_exception);
}

TEST_F(ConfigTest, EmptyFile) {
    WriteConfigFile("");

    EXPECT_THROW(config cfg(test_config_path), config_exception);
}

TEST_F(ConfigTest, WrongDataTypes) {
    WriteConfigFile(R"({
        "server_ip": 127,
        "server_port": "not_a_number",
        "blacklist": "not_an_array"
    })");

    EXPECT_THROW(config cfg(test_config_path), config_exception);
}
