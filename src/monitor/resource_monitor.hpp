#pragma once

#include <string>
#include <map>
#include <cstdint>

namespace linuxguard {

struct ResourceStats {
    float cpu_percent = 0.0f;
    long rss_kb = 0;          // Resident Set Size in KB
    long virtual_kb = 0;      // Virtual memory in KB
    int num_threads = 0;
    std::string state;        // R, S, D, Z, etc.
};

class ResourceMonitor {
public:
    static ResourceStats get_stats(pid_t pid);
    static bool check_limits(pid_t pid, float cpu_limit, long ram_limit_mb);
};

} // namespace linuxguard
