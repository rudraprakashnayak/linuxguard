#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

#include "../config/config_parser.hpp"
#include "../process/process_manager.hpp"
#include "../dependency/dependency_graph.hpp"
#include "../ipc/unix_socket.hpp"

namespace linuxguard {

class Daemon {
public:
    Daemon() = default;
    ~Daemon();

    bool init(const std::string& config_path);
    void run();
    void shutdown();

    bool is_running() const { return running_; }

    // For testing
    std::string handle_command(const std::string& cmd);

private:
    std::atomic<bool> running_{false};
    GlobalConfig config_;
    ProcessManager process_mgr_;
    DependencyGraph dep_graph_;
    UnixSocketServer ipc_server_;
    std::thread monitor_thread_;
    std::thread restart_thread_;

    void setup_signals();
    void start_services();
    void monitor_loop();
    void restart_loop();
    void write_pid_file();
    void remove_pid_file();

    static std::atomic<bool> shutdown_requested_;
};

} // namespace linuxguard
