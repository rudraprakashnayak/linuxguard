#include "process_manager.hpp"
#include "../logging/logger.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

#ifndef _WIN32
#include <pwd.h>
#endif

namespace linuxguard {

std::string state_to_string(ProcessState state) {
    switch (state) {
        case ProcessState::STOPPED:  return "stopped";
        case ProcessState::STARTING: return "starting";
        case ProcessState::RUNNING:  return "running";
        case ProcessState::STOPPING: return "stopping";
        case ProcessState::FAILED:   return "failed";
        case ProcessState::BACKOFF:  return "backoff";
    }
    return "unknown";
}

void ProcessManager::start_process(const std::string& name,
                                   const std::string& command,
                                   const std::string& working_dir,
                                   const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = processes_.find(name);
    if (it != processes_.end() && it->second.state == ProcessState::RUNNING) {
        Logger::instance().warn("Process '" + name + "' is already running");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        Logger::instance().error("fork() failed for '" + name + "': " +
                                 std::strerror(errno));
        if (it != processes_.end()) it->second.state = ProcessState::FAILED;
        return;
    }

    if (pid == 0) {
        // Child process
        setsid();

        if (!working_dir.empty() && working_dir != "/") {
            if (chdir(working_dir.c_str()) != 0) {
                _exit(127);
            }
        }

#ifndef _WIN32
        if (!user.empty()) {
            struct passwd* pw = getpwnam(user.c_str());
            if (pw) {
                if (setgid(pw->pw_gid) != 0) _exit(127);
                if (setuid(pw->pw_uid) != 0) _exit(127);
            }
        }
#endif

        // Close standard file descriptors
        close(STDIN_FILENO);
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) close(devnull);
        }

        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(127);
    }

    // Parent
    ProcessInfo info;
    info.name = name;
    info.pid = pid;
    info.state = ProcessState::RUNNING;
    info.last_start = std::chrono::steady_clock::now();
    processes_[name] = info;

    Logger::instance().info("Started process '" + name + "' with PID " +
                            std::to_string(pid));
}

void ProcessManager::stop_process(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = processes_.find(name);
    if (it == processes_.end() || it->second.state != ProcessState::RUNNING) {
        Logger::instance().warn("Process '" + name + "' is not running");
        return;
    }

    pid_t pid = it->second.pid;
    it->second.state = ProcessState::STOPPING;

    kill(pid, SIGTERM);

    // Give it a moment, then force kill
    int status = 0;
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        // Still running after SIGTERM, send SIGKILL
        usleep(500000); // 500ms
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }

    it->second.pid = -1;
    it->second.state = ProcessState::STOPPED;
    it->second.last_stop = std::chrono::steady_clock::now();
    Logger::instance().info("Stopped process '" + name + "' (PID " +
                            std::to_string(pid) + ")");
}

void ProcessManager::stop_all() {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, info] : processes_) {
            if (info.state == ProcessState::RUNNING)
                names.push_back(name);
        }
    }
    // Stop in reverse order (dependencies first)
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        stop_process(*it);
    }
}

bool ProcessManager::is_running(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(name);
    return it != processes_.end() && it->second.state == ProcessState::RUNNING;
}

ProcessInfo ProcessManager::get_info(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(name);
    if (it == processes_.end()) {
        ProcessInfo info;
        info.name = name;
        info.state = ProcessState::STOPPED;
        return info;
    }
    return it->second;
}

std::vector<ProcessInfo> ProcessManager::list_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessInfo> result;
    for (auto& [name, info] : processes_) {
        result.push_back(info);
    }
    return result;
}

void ProcessManager::check_processes(
    std::function<void(const std::string&)> on_failure) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, info] : processes_) {
        if (info.state != ProcessState::RUNNING) continue;

        int status = 0;
        pid_t result = waitpid(info.pid, &status, WNOHANG);
        if (result < 0) {
            if (errno == ECHILD) {
                info.state = ProcessState::STOPPED;
                info.pid = -1;
                Logger::instance().warn("Process '" + name + "' no longer exists");
                on_failure(name);
            }
        } else if (result > 0) {
            info.state = ProcessState::STOPPED;
            info.pid = -1;
            info.last_stop = std::chrono::steady_clock::now();
            Logger::instance().warn("Process '" + name + "' exited with status " +
                                    std::to_string(WEXITSTATUS(status)));
            on_failure(name);
        }
    }
}

void ProcessManager::set_restart_policy(const std::string& name,
                                        bool auto_restart, int max_restarts,
                                        int backoff_sec, int backoff_max_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    RestartPolicy policy;
    policy.auto_restart = auto_restart;
    policy.max_restarts = max_restarts;
    policy.backoff_sec = backoff_sec;
    policy.backoff_max_sec = backoff_max_sec;
    policies_[name] = policy;
}

void ProcessManager::attempt_restarts(
    std::function<void(const std::string&)> start_fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    for (auto& [name, info] : processes_) {
        auto pit = policies_.find(name);
        if (pit == policies_.end() || !pit->second.auto_restart) continue;
        if (info.state != ProcessState::STOPPED &&
            info.state != ProcessState::FAILED) continue;
        if (info.restart_count >= pit->second.max_restarts) continue;

        if (info.state == ProcessState::BACKOFF) {
            if (now < info.next_restart) continue;
        }

        int backoff = pit->second.backoff_sec * (1 << info.restart_count);
        if (backoff > pit->second.backoff_max_sec)
            backoff = pit->second.backoff_max_sec;

        info.restart_count++;
        info.state = ProcessState::BACKOFF;
        info.next_restart = now + std::chrono::seconds(backoff);

        Logger::instance().info("Scheduling restart #" +
                                std::to_string(info.restart_count) +
                                " for '" + name + "' in " +
                                std::to_string(backoff) + "s");

        // Start immediately; backoff is tracked for logging
        info.state = ProcessState::RUNNING;
        start_fn(name);
    }
}

void ProcessManager::wait_nonblocking(pid_t pid) {
    int status = 0;
    waitpid(pid, &status, WNOHANG);
}

} // namespace linuxguard
