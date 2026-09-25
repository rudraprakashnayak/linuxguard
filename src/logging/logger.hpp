#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace linuxguard {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    void set_output(const std::string& path);
    void log(LogLevel level, const std::string& msg);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string timestamp() const;
    std::string level_str(LogLevel level) const;

    std::mutex mutex_;
    LogLevel level_ = LogLevel::INFO;
    std::ofstream file_;
    bool use_file_ = false;
};

} // namespace linuxguard
