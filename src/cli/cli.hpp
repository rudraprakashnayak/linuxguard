#pragma once

#include <string>
#include <vector>

namespace linuxguard {

class CLI {
public:
    static int run(int argc, char* argv[]);

private:
    static void print_usage();
    static std::string get_socket_path();
};

} // namespace linuxguard
