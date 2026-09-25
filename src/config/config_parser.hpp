#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>

namespace linuxguard {

struct ServiceConfig {
    std::string name;
    std::string command;
    std::string working_dir = "/";
    std::string user;
    bool auto_restart = true;
    int max_restarts = 5;
    int restart_backoff_sec = 3;
    int restart_backoff_max_sec = 60;
    float cpu_limit = 0.0f;       // 0 = unlimited
    long ram_limit_mb = 0;        // 0 = unlimited
    std::vector<std::string> depends_on;
};

struct GlobalConfig {
    std::string socket_path = "/tmp/linuxguard.sock";
    std::string pid_file = "/tmp/linuxguard.pid";
    std::string log_file;
    std::string log_level = "info";
    int monitor_interval_sec = 5;
    std::vector<ServiceConfig> services;
};

class ConfigParser {
public:
    static GlobalConfig parse(const std::string& path);

private:
    static void parse_service(const std::vector<std::string>& lines,
                              size_t start, size_t end,
                              std::vector<ServiceConfig>& services);
    static std::string trim(const std::string& s);
    static bool starts_with(const std::string& s, const std::string& prefix);
    static std::map<std::string, std::string>
        parse_block(const std::vector<std::string>& lines,
                    size_t start, size_t end);
};

} // namespace linuxguard
