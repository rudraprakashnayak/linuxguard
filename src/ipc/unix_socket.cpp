#include "unix_socket.hpp"
#include "../logging/logger.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>

namespace linuxguard {

UnixSocketServer::~UnixSocketServer() {
    stop();
}

bool UnixSocketServer::start(const std::string& path) {
    socket_path_ = path;

    // Remove stale socket file
    unlink(path.c_str());

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        Logger::instance().error("socket() failed: " + std::string(strerror(errno)));
        return false;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        Logger::instance().error("bind() failed: " + std::string(strerror(errno)));
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 5) < 0) {
        Logger::instance().error("listen() failed: " + std::string(strerror(errno)));
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    accept_thread_ = std::thread(&UnixSocketServer::accept_loop, this);
    Logger::instance().info("IPC server listening on " + path);
    return true;
}

void UnixSocketServer::stop() {
    if (!running_) return;
    running_ = false;

    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    unlink(socket_path_.c_str());
    Logger::instance().info("IPC server stopped");
}

void UnixSocketServer::set_handler(
    std::function<std::string(const std::string&)> handler) {
    handler_ = std::move(handler);
}

void UnixSocketServer::accept_loop() {
    while (running_) {
        struct sockaddr_un client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // Use a timeout via select to allow checking running_ flag
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);

        struct timeval tv{1, 0}; // 1 second timeout
        int ready = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &tv);

        if (ready <= 0) continue;
        if (!running_) break;

        int client_fd = accept(server_fd_,
                               reinterpret_cast<struct sockaddr*>(&client_addr),
                               &client_len);
        if (client_fd < 0) {
            if (running_)
                Logger::instance().error("accept() failed: " +
                                         std::string(strerror(errno)));
            continue;
        }

        // Read request
        char buffer[4096] = {};
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            std::string request(buffer, n);
            std::string response;
            if (handler_) {
                response = handler_(request);
            } else {
                response = "ERROR: no handler registered";
            }

            write(client_fd, response.c_str(), response.size());
        }

        close(client_fd);
    }
}

std::string UnixSocketClient::send_command(const std::string& path,
                                           const std::string& command) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return "ERROR: cannot create socket: " + std::string(strerror(errno));
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr),
                sizeof(addr)) < 0) {
        close(fd);
        return "ERROR: cannot connect to daemon at " + path +
               ": " + std::string(strerror(errno));
    }

    write(fd, command.c_str(), command.size());

    char buffer[4096] = {};
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n > 0) {
        return std::string(buffer, n);
    }
    return "ERROR: no response from daemon";
}

} // namespace linuxguard
