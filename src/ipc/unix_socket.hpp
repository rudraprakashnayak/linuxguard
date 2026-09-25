#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace linuxguard {

class UnixSocketServer {
public:
    UnixSocketServer() = default;
    ~UnixSocketServer();

    bool start(const std::string& path);
    void stop();
    bool is_running() const { return running_; }

    void set_handler(std::function<std::string(const std::string&)> handler);

private:
    int server_fd_ = -1;
    std::string socket_path_;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::function<std::string(const std::string&)> handler_;

    void accept_loop();
};

class UnixSocketClient {
public:
    static std::string send_command(const std::string& path,
                                    const std::string& command);
};

} // namespace linuxguard
