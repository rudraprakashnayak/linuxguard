#include "daemon/daemon.hpp"
#include "cli/cli.hpp"
#include "logging/logger.hpp"

#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        linuxguard::CLI::print_usage();
        return 1;
    }

    std::string first_arg = argv[1];

    if (first_arg == "--daemon") {
        if (argc < 3) {
            std::cerr << "ERROR: --daemon requires a config file path\n";
            return 1;
        }
        std::string config_path = argv[2];

        linuxguard::Daemon daemon;
        if (!daemon.init(config_path)) {
            std::cerr << "Failed to initialize daemon\n";
            return 1;
        }
        daemon.run();
        return 0;
    }

    return linuxguard::CLI::run(argc, argv);
}
