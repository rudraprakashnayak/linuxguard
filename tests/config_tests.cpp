#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "../src/config/config_parser.hpp"

using namespace linuxguard;

void test_parse_basic_config() {
    std::cout << "Test: parse basic config... ";

    // Create a temporary config file
    std::string path = "/tmp/test_config.service";
    {
        std::ofstream ofs(path);
        ofs << "[global]\n"
            << "socket_path = /tmp/test.sock\n"
            << "pid_file = /tmp/test.pid\n"
            << "log_level = debug\n"
            << "monitor_interval_sec = 10\n"
            << "\n"
            << "[service test_svc]\n"
            << "name = test_svc\n"
            << "command = echo hello\n"
            << "working_dir = /tmp\n"
            << "auto_restart = true\n"
            << "max_restarts = 3\n"
            << "restart_backoff_sec = 5\n";
    }

    auto cfg = ConfigParser::parse(path);
    assert(cfg.socket_path == "/tmp/test.sock");
    assert(cfg.pid_file == "/tmp/test.pid");
    assert(cfg.log_level == "debug");
    assert(cfg.monitor_interval_sec == 10);
    assert(cfg.services.size() == 1);
    assert(cfg.services[0].name == "test_svc");
    assert(cfg.services[0].command == "echo hello");
    assert(cfg.services[0].working_dir == "/tmp");
    assert(cfg.services[0].auto_restart == true);
    assert(cfg.services[0].max_restarts == 3);
    assert(cfg.services[0].restart_backoff_sec == 5);

    std::filesystem::remove(path);
    std::cout << "PASSED\n";
}

void test_parse_dependencies() {
    std::cout << "Test: parse dependencies... ";

    std::string path = "/tmp/test_deps.service";
    {
        std::ofstream ofs(path);
        ofs << "[service svc_a]\n"
            << "name = svc_a\n"
            << "command = sleep 10\n"
            << "\n"
            << "[service svc_b]\n"
            << "name = svc_b\n"
            << "command = sleep 10\n"
            << "depends_on = svc_a\n"
            << "\n"
            << "[service svc_c]\n"
            << "name = svc_c\n"
            << "command = sleep 10\n"
            << "depends_on = svc_a, svc_b\n";
    }

    auto cfg = ConfigParser::parse(path);
    assert(cfg.services.size() == 3);
    assert(cfg.services[0].depends_on.empty());
    assert(cfg.services[1].depends_on.size() == 1);
    assert(cfg.services[1].depends_on[0] == "svc_a");
    assert(cfg.services[2].depends_on.size() == 2);
    assert(cfg.services[2].depends_on[0] == "svc_a");
    assert(cfg.services[2].depends_on[1] == "svc_b");

    std::filesystem::remove(path);
    std::cout << "PASSED\n";
}

void test_parse_invalid_file() {
    std::cout << "Test: parse invalid file... ";

    bool threw = false;
    try {
        ConfigParser::parse("/tmp/nonexistent_file.service");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    std::cout << "PASSED\n";
}

void test_parse_resource_limits() {
    std::cout << "Test: parse resource limits... ";

    std::string path = "/tmp/test_limits.service";
    {
        std::ofstream ofs(path);
        ofs << "[service limited]\n"
            << "name = limited\n"
            << "command = sleep 10\n"
            << "cpu_limit = 80.0\n"
            << "ram_limit_mb = 512\n";
    }

    auto cfg = ConfigParser::parse(path);
    assert(cfg.services.size() == 1);
    assert(cfg.services[0].cpu_limit == 80.0f);
    assert(cfg.services[0].ram_limit_mb == 512);

    std::filesystem::remove(path);
    std::cout << "PASSED\n";
}

void test_parse_comments() {
    std::cout << "Test: parse comments... ";

    std::string path = "/tmp/test_comments.service";
    {
        std::ofstream ofs(path);
        ofs << "# This is a comment\n"
            << "[global]\n"
            << "# Another comment\n"
            << "socket_path = /tmp/test.sock\n"
            << "\n"
            << "[service test]\n"
            << "name = test\n"
            << "command = echo test # inline comment\n";
    }

    auto cfg = ConfigParser::parse(path);
    assert(cfg.socket_path == "/tmp/test.sock");
    assert(cfg.services.size() == 1);
    assert(cfg.services[0].name == "test");

    std::filesystem::remove(path);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Config Parser Tests ===\n";

    test_parse_basic_config();
    test_parse_dependencies();
    test_parse_invalid_file();
    test_parse_resource_limits();
    test_parse_comments();

    std::cout << "\nAll config parser tests passed!\n";
    return 0;
}
