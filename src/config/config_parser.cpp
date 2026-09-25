#include "config_parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace linuxguard {

std::string ConfigParser::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ConfigParser::starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::map<std::string, std::string>
ConfigParser::parse_block(const std::vector<std::string>& lines,
                          size_t start, size_t end) {
    std::map<std::string, std::string> result;
    for (size_t i = start; i < end; ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || starts_with(line, "#")) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        result[key] = val;
    }
    return result;
}

void ConfigParser::parse_service(const std::vector<std::string>& lines,
                                 size_t start, size_t end,
                                 std::vector<ServiceConfig>& services) {
    auto kv = parse_block(lines, start, end);
    if (kv.find("name") == kv.end()) return;

    ServiceConfig svc;
    svc.name = kv["name"];
    svc.command = kv.count("command") ? kv["command"] : "";
    svc.working_dir = kv.count("working_dir") ? kv["working_dir"] : "/";
    svc.user = kv.count("user") ? kv["user"] : "";

    if (kv.count("auto_restart"))
        svc.auto_restart = (kv["auto_restart"] == "true" || kv["auto_restart"] == "1");
    if (kv.count("max_restarts"))
        svc.max_restarts = std::stoi(kv["max_restarts"]);
    if (kv.count("restart_backoff_sec"))
        svc.restart_backoff_sec = std::stoi(kv["restart_backoff_sec"]);
    if (kv.count("restart_backoff_max_sec"))
        svc.restart_backoff_max_sec = std::stoi(kv["restart_backoff_max_sec"]);
    if (kv.count("cpu_limit"))
        svc.cpu_limit = std::stof(kv["cpu_limit"]);
    if (kv.count("ram_limit_mb"))
        svc.ram_limit_mb = std::stol(kv["ram_limit_mb"]);

    if (kv.count("depends_on")) {
        std::istringstream iss(kv["depends_on"]);
        std::string dep;
        while (std::getline(iss, dep, ',')) {
            dep = trim(dep);
            if (!dep.empty()) svc.depends_on.push_back(dep);
        }
    }

    services.push_back(std::move(svc));
}

GlobalConfig ConfigParser::parse(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);

    GlobalConfig cfg;
    size_t i = 0;
    while (i < lines.size()) {
        std::string trimmed = trim(lines[i]);
        if (trimmed.empty() || starts_with(trimmed, "#")) { ++i; continue; }

        if (starts_with(trimmed, "[global]")) {
            size_t start = i + 1;
            while (i + 1 < lines.size() && !starts_with(trim(lines[i + 1]), "["))
                ++i;
            auto kv = parse_block(lines, start, i + 1);
            if (kv.count("socket_path")) cfg.socket_path = kv["socket_path"];
            if (kv.count("pid_file")) cfg.pid_file = kv["pid_file"];
            if (kv.count("log_file")) cfg.log_file = kv["log_file"];
            if (kv.count("log_level")) cfg.log_level = kv["log_level"];
            if (kv.count("monitor_interval_sec"))
                cfg.monitor_interval_sec = std::stoi(kv["monitor_interval_sec"]);
            ++i;
        } else if (starts_with(trimmed, "[service")) {
            size_t start = i + 1;
            while (i + 1 < lines.size() && !starts_with(trim(lines[i + 1]), "["))
                ++i;
            parse_service(lines, start, i + 1, cfg.services);
            ++i;
        } else {
            ++i;
        }
    }

    return cfg;
}

} // namespace linuxguard
