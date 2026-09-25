#include "daemon.hpp"
#include "../logging/logger.hpp"
#include "../monitor/resource_monitor.hpp"

#include <csignal>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>

namespace linuxguard {

std::atomic<bool> Daemon::shutdown_requested_{false};

Daemon::~Daemon() {
    shutdown();
}

void Daemon::setup_signals() {
    struct sigaction sa{};
    sa.sa_handler = [](int) {
        shutdown_requested_ = true;
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Ignore SIGPIPE from socket operations
    signal(SIGPIPE, SIG_IGN);
}

bool Daemon::init(const std::string& config_path) {
    try {
        config_ = ConfigParser::parse(config_path);
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to parse config: " + std::string(e.what()));
        return false;
    }

    // Configure logging
    if (!config_.log_file.empty()) {
        Logger::instance().set_output(config_.log_file);
    }
    if (config_.log_level == "debug") Logger::instance().set_level(LogLevel::DEBUG);
    else if (config_.log_level == "warn") Logger::instance().set_level(LogLevel::WARN);
    else if (config_.log_level == "error") Logger::instance().set_level(LogLevel::ERROR);

    // Build dependency graph
    for (auto& svc : config_.services) {
        dep_graph_.add_service(svc.name, svc.depends_on);
    }

    if (!dep_graph_.validate()) {
        auto cycle_node = dep_graph_.detect_cycle();
        Logger::instance().error("Dependency cycle detected involving: " +
                                 cycle_node.value_or("unknown"));
        return false;
    }

    // Set restart policies
    for (auto& svc : config_.services) {
        process_mgr_.set_restart_policy(svc.name, svc.auto_restart,
                                        svc.max_restarts, svc.restart_backoff_sec,
                                        svc.restart_backoff_max_sec);
    }

    setup_signals();
    return true;
}

void Daemon::run() {
    running_ = true;
    write_pid_file();

    // Start IPC server
    ipc_server_.set_handler(
        [this](const std::string& cmd) { return handle_command(cmd); });
    if (!ipc_server_.start(config_.socket_path)) {
        Logger::instance().error("Failed to start IPC server");
        running_ = false;
        return;
    }

    // Start services in dependency order
    start_services();

    // Start monitor thread
    monitor_thread_ = std::thread(&Daemon::monitor_loop, this);

    // Start restart thread
    restart_thread_ = std::thread(&Daemon::restart_loop, this);

    Logger::instance().info("Daemon started, managing " +
                            std::to_string(config_.services.size()) + " services");

    // Main loop - wait for shutdown signal
    while (running_ && !shutdown_requested_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::instance().info("Shutdown signal received");
    shutdown();
}

void Daemon::shutdown() {
    if (!running_) return;
    running_ = false;
    shutdown_requested_ = false;

    // Stop monitor and restart threads
    if (monitor_thread_.joinable()) monitor_thread_.join();
    if (restart_thread_.joinable()) restart_thread_.join();

    // Stop IPC server
    ipc_server_.stop();

    // Stop all managed processes in reverse dependency order
    auto stop_order = dep_graph_.reverse_topological_order();
    for (auto& name : stop_order) {
        if (process_mgr_.is_running(name)) {
            process_mgr_.stop_process(name);
        }
    }

    remove_pid_file();
    Logger::instance().info("Daemon shutdown complete");
}

void Daemon::start_services() {
    auto start_order = dep_graph_.topological_order();
    for (auto& name : start_order) {
        // Find the service config
        for (auto& svc : config_.services) {
            if (svc.name == name) {
                // Check if dependencies are running
                bool deps_ok = true;
                for (auto& dep : svc.depends_on) {
                    if (!process_mgr_.is_running(dep)) {
                        Logger::instance().warn("Dependency '" + dep +
                                                "' not running, skipping '" + name + "'");
                        deps_ok = false;
                        break;
                    }
                }
                if (deps_ok) {
                    process_mgr_.start_process(svc.name, svc.command,
                                               svc.working_dir, svc.user);
                }
                break;
            }
        }
    }
}

void Daemon::monitor_loop() {
    while (running_) {
        // Check process health
        process_mgr_.check_processes([this](const std::string& name) {
            Logger::instance().warn("Process '" + name + "' failed, will attempt restart");
        });

        // Check resource limits
        for (auto& svc : config_.services) {
            if (process_mgr_.is_running(svc.name)) {
                auto info = process_mgr_.get_info(svc.name);
                if (info.pid > 0) {
                    ResourceMonitor::check_limits(info.pid, svc.cpu_limit,
                                                  svc.ram_limit_mb);
                }
            }
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(config_.monitor_interval_sec));
    }
}

void Daemon::restart_loop() {
    while (running_) {
        process_mgr_.attempt_restarts([this](const std::string& name) {
            // Find service config and restart
            for (auto& svc : config_.services) {
                if (svc.name == name) {
                    // Check dependencies
                    bool deps_ok = true;
                    for (auto& dep : svc.depends_on) {
                        if (!process_mgr_.is_running(dep)) {
                            deps_ok = false;
                            break;
                        }
                    }
                    if (deps_ok) {
                        process_mgr_.start_process(svc.name, svc.command,
                                                   svc.working_dir, svc.user);
                    } else {
                        Logger::instance().warn("Cannot restart '" + name +
                                                "': dependencies not running");
                    }
                    break;
                }
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string Daemon::handle_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string action;
    iss >> action;

    if (action == "status") {
        std::string name;
        iss >> name;
        if (name.empty()) {
            return "ERROR: usage: status <name>";
        }
        auto info = process_mgr_.get_info(name);
        std::ostringstream oss;
        oss << "name=" << info.name << "\n"
            << "state=" << state_to_string(info.state) << "\n"
            << "pid=" << info.pid << "\n"
            << "restarts=" << info.restart_count;
        return oss.str();
    } else if (action == "list") {
        auto all = process_mgr_.list_all();
        std::ostringstream oss;
        for (auto& info : all) {
            oss << info.name << " " << state_to_string(info.state)
                << " pid=" << info.pid << "\n";
        }
        if (all.empty()) oss << "(no services configured)\n";
        return oss.str();
    } else if (action == "start") {
        std::string name;
        iss >> name;
        if (name.empty()) return "ERROR: usage: start <name>";
        for (auto& svc : config_.services) {
            if (svc.name == name) {
                process_mgr_.start_process(svc.name, svc.command,
                                           svc.working_dir, svc.user);
                return "OK: started " + name;
            }
        }
        return "ERROR: unknown service '" + name + "'";
    } else if (action == "stop") {
        std::string name;
        iss >> name;
        if (name.empty()) return "ERROR: usage: stop <name>";
        process_mgr_.stop_process(name);
        return "OK: stopped " + name;
    } else if (action == "restart") {
        std::string name;
        iss >> name;
        if (name.empty()) return "ERROR: usage: restart <name>";
        process_mgr_.stop_process(name);
        for (auto& svc : config_.services) {
            if (svc.name == name) {
                process_mgr_.start_process(svc.name, svc.command,
                                           svc.working_dir, svc.user);
                return "OK: restarted " + name;
            }
        }
        return "ERROR: unknown service '" + name + "'";
    } else if (action == "reload") {
        return "OK: reload not yet implemented";
    } else if (action == "shutdown") {
        running_ = false;
        shutdown_requested_ = true;
        return "OK: shutting down";
    }

    return "ERROR: unknown command '" + action + "'";
}

void Daemon::write_pid_file() {
    std::ofstream ofs(config_.pid_file);
    if (ofs.is_open()) {
        ofs << getpid() << "\n";
    }
}

void Daemon::remove_pid_file() {
    unlink(config_.pid_file.c_str());
}

} // namespace linuxguard
