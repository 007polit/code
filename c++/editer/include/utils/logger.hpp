#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace editer {
namespace utils {

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Simple logger class (singleton)
class Logger {
private:
    std::ofstream log_file_;
    std::mutex mutex_;
    LogLevel min_level_;
    bool enabled_;

    Logger() : min_level_(LogLevel::INFO), enabled_(true) {
        log_file_.open("editer.log", std::ios::app);
    }

    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    // Get current timestamp
    std::string get_timestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // Get level string
    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }

public:
    // Singleton instance
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // Delete copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Set minimum log level
    void set_level(LogLevel level) {
        min_level_ = level;
    }

    // Enable/disable logging
    void set_enabled(bool enabled) {
        enabled_ = enabled;
    }

    // Log message
    void log(LogLevel level, const std::string& message) {
        if (!enabled_ || level < min_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        if (log_file_.is_open()) {
            log_file_ << "[" << get_timestamp() << "] "
                     << "[" << level_to_string(level) << "] "
                     << message << std::endl;
            log_file_.flush();
        }
    }

    // Convenience methods
    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }
};

// Convenience macros
#define LOG_DEBUG(msg)   editer::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg)    editer::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg) editer::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg)   editer::utils::Logger::instance().error(msg)

} // namespace utils
} // namespace editer
