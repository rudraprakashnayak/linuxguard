#include "resource_monitor.hpp"
#include "../logging/logger.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

namespace linuxguard {

ResourceStats ResourceMonitor::get_stats(pid_t pid) {
    ResourceStats stats;

    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";

    std::ifstream stat_file(stat_path);
    if (!stat_file.is_open()) return stats;

    // Parse /proc/[pid]/stat
    // Format: pid (comm) state ppid pgroup session tty_nr tpgid flags
    //         minflt cminflt majflt cmajflt utime stime cutime cstime ...
    std::string content;
    std::getline(stat_file, content);

    // Find the closing parenthesis of comm (which may contain spaces/parens)
    auto close_paren = content.rfind(')');
    if (close_paren == std::string::npos) return stats;

    std::istringstream iss(content.substr(close_paren + 2));
    char state_char;
    iss >> state_char;
    stats.state = std::string(1, state_char);

    // Skip fields: ppid, pgroup, session, tty_nr, tpgid, flags,
    // minflt, cminflt, majflt, cmajflt
    std::string skip;
    for (int i = 0; i < 10; ++i) iss >> skip;

    long utime, stime;
    iss >> utime >> stime;

    long total_time = utime + stime;
    long clk_tck = sysconf(_SC_CLK_TCK);
    if (clk_tck > 0 && total_time > 0) {
        // Approximate CPU% over process lifetime
        // For a real monitor, we'd sample twice; this is a snapshot estimate
        stats.cpu_percent = static_cast<float>(total_time) / clk_tck;
    }

    // Parse /proc/[pid]/status for memory info
    std::ifstream status_file(status_path);
    if (status_file.is_open()) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.find("VmRSS:") == 0) {
                std::istringstream liss(line);
                std::string label;
                long val;
                liss >> label >> val;
                stats.rss_kb = val;
            } else if (line.find("VmSize:") == 0) {
                std::istringstream liss(line);
                std::string label;
                long val;
                liss >> label >> val;
                stats.virtual_kb = val;
            } else if (line.find("Threads:") == 0) {
                std::istringstream liss(line);
                std::string label;
                int val;
                liss >> label >> val;
                stats.num_threads = val;
            }
        }
    }

    return stats;
}

bool ResourceMonitor::check_limits(pid_t pid, float cpu_limit,
                                   long ram_limit_mb) {
    if (pid <= 0) return true;

    ResourceStats stats = get_stats(pid);
    bool ok = true;

    if (cpu_limit > 0 && stats.cpu_percent > cpu_limit) {
        Logger::instance().warn("Process " + std::to_string(pid) +
                                " CPU usage " +
                                std::to_string(static_cast<int>(stats.cpu_percent)) +
                                "s exceeds limit " +
                                std::to_string(static_cast<int>(cpu_limit)) + "s");
        ok = false;
    }

    if (ram_limit_mb > 0 && stats.rss_kb > ram_limit_mb * 1024) {
        Logger::instance().warn("Process " + std::to_string(pid) +
                                " RAM usage " +
                                std::to_string(stats.rss_kb / 1024) +
                                "MB exceeds limit " +
                                std::to_string(ram_limit_mb) + "MB");
        ok = false;
    }

    return ok;
}

} // namespace linuxguard
