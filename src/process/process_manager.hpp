#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>
#include <sys/types.h>

namespace linuxguard {

enum class ProcessState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    FAILED,
    BACKOFF
};

struct ProcessInfo {
    std::string name;
    pid_t pid = -1;
    ProcessState state = ProcessState::STOPPED;
    int restart_count = 0;
    std::chrono::steady_clock::time_point last_start;
    std::chrono::steady_clock::time_point last_stop;
    std::chrono::steady_clock::time_point next_restart;
};

std::string state_to_string(ProcessState state);

class ProcessManager {
public:
    ProcessManager() = default;

    void start_process(const std::string& name, const std::string& command,
                       const std::string& working_dir, const std::string& user);
    void stop_process(const std::string& name);
    void stop_all();

    bool is_running(const std::string& name) const;
    ProcessInfo get_info(const std::string& name) const;
    std::vector<ProcessInfo> list_all() const;

    void check_processes(std::function<void(const std::string&)> on_failure);

    void set_restart_policy(const std::string& name, bool auto_restart,
                            int max_restarts, int backoff_sec, int backoff_max_sec);

    void attempt_restarts(std::function<void(const std::string&)> start_fn);

private:
    mutable std::mutex mutex_;
    std::map<std::string, ProcessInfo> processes_;

    struct RestartPolicy {
        bool auto_restart = true;
        int max_restarts = 5;
        int backoff_sec = 3;
        int backoff_max_sec = 60;
    };
    std::map<std::string, RestartPolicy> policies_;

    void wait_nonblocking(pid_t pid);
};

} // namespace linuxguard
