#include "cli.hpp"
#include "../ipc/unix_socket.hpp"
#include "../config/config_parser.hpp"
#include "../logging/logger.hpp"

#include <iostream>
#include <fstream>
#include <string>

namespace linuxguard {

void CLI::print_usage() {
    std::cout << "LinuxGuard - Linux Process Supervisor\n"
              << "\n"
              << "Usage:\n"
              << "  linuxguard --daemon <config>   Start the daemon\n"
              << "  linuxguard status <name>       Show service status\n"
              << "  linuxguard list                List all services\n"
              << "  linuxguard start <name>        Start a service\n"
              << "  linuxguard stop <name>         Stop a service\n"
              << "  linuxguard restart <name>      Restart a service\n"
              << "  linuxguard reload              Reload configuration\n"
              << "  linuxguard shutdown            Stop the daemon\n"
              << "\n"
              << "Options:\n"
              << "  --socket <path>  Override socket path (default: /tmp/linuxguard.sock)\n"
              << "  --help           Show this help\n";
}

std::string CLI::get_socket_path() {
    // Check for --socket flag in environment or default
    const char* env = std::getenv("LINUXGUARD_SOCKET");
    if (env) return env;
    return "/tmp/linuxguard.sock";
}

int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        print_usage();
        return 0;
    }

    if (cmd == "--daemon") {
        if (argc < 3) {
            std::cerr << "ERROR: --daemon requires a config file path\n";
            return 1;
        }
        std::string config_path = argv[2];

        // Find socket path from config
        std::string socket_path = "/tmp/linuxguard.sock";
        try {
            auto cfg = ConfigParser::parse(config_path);
            socket_path = cfg.socket_path;
        } catch (...) {}

        // Check for --socket override
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--socket" && i + 1 < argc) {
                socket_path = argv[i + 1];
            }
        }

        Daemon daemon;
        if (!daemon.init(config_path)) {
            std::cerr << "Failed to initialize daemon\n";
            return 1;
        }
        daemon.run();
        return 0;
    }

    // CLI commands - connect to daemon via IPC
    std::string socket_path = get_socket_path();

    // Check for --socket override
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--socket" && i + 1 < argc) {
            socket_path = argv[i + 1];
        }
    }

    std::string ipc_cmd = cmd;
    if (argc > 2 && std::string(argv[2]) != "--socket") {
        ipc_cmd += " " + std::string(argv[2]);
    }

    std::string response = UnixSocketClient::send_command(socket_path, ipc_cmd);
    std::cout << response << "\n";

    if (response.find("ERROR") == 0) return 1;
    return 0;
}

} // namespace linuxguard
