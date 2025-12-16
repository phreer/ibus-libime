#ifndef LOGGER_H
#define LOGGER_H

#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger &getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        logLevel_ = level;
    }

    LogLevel getLogLevel() const { return logLevel_; }

    const std::string &getLogPath() const { return logPath_; }

    void log(const std::string &level, LogLevel levelEnum, const std::string &message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (levelEnum < logLevel_) {
            return;  // Skip logs below current log level
        }
        if (logFile_.is_open()) {
            logFile_ << std::format("{} [{}] {}\n", getCurrentTime(), level, message);
            logFile_.flush();
        }
    }

    void debug(const std::string &message) { log("DEBUG", LogLevel::DEBUG, message); }
    void info(const std::string &message) { log("INFO", LogLevel::INFO, message); }
    void warn(const std::string &message) { log("WARN", LogLevel::WARN, message); }
    void error(const std::string &message) { log("ERROR", LogLevel::ERROR, message); }

private:
    Logger() : logLevel_(LogLevel::WARN) {
        // Determine log level from environment variable
        const char* logLevelEnv = std::getenv("IBUS_LIBIME_LOG_LEVEL");
        if (logLevelEnv) {
            std::string levelStr(logLevelEnv);
            if (levelStr == "DEBUG") {
                logLevel_ = LogLevel::DEBUG;
            } else if (levelStr == "INFO") {
                logLevel_ = LogLevel::INFO;
            } else if (levelStr == "WARN") {
                logLevel_ = LogLevel::WARN;
            } else if (levelStr == "ERROR") {
                logLevel_ = LogLevel::ERROR;
            }
        }

        // Determine log file path
        logPath_ = getLogFilePath();
        
        // Create directory if it doesn't exist
        size_t lastSlash = logPath_.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = logPath_.substr(0, lastSlash);
            createDirectory(dir);
        }

        logFile_.open(logPath_, std::ios::app);
        if (logFile_.is_open()) {
            info("=== IBus LibIME Engine Started ===");
        }
    }

    ~Logger() {
        if (logFile_.is_open()) {
            info("=== IBus LibIME Engine Stopped ===");
            logFile_.close();
        }
    }

    std::string getLogFilePath() {
        const char* home = std::getenv("HOME");
        if (!home) {
            home = std::getenv("USER");
            if (home) {
                return std::format("/home/{}/.local/share/ibus-libime/ibus-libime.log", home);
            }
            return "/tmp/ibus-libime.log";  // Fallback
        }
        
        return std::format("{}/.local/share/ibus-libime/ibus-libime.log", home);
    }

    void createDirectory(const std::string &path) {
        std::string currentPath;
        for (size_t i = 0; i < path.length(); ++i) {
            if (path[i] == '/' && i > 0) {
                mkdir(currentPath.c_str(), 0755);
            }
            currentPath += path[i];
        }
        mkdir(currentPath.c_str(), 0755);
    }

    std::string getCurrentTime() {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    std::ofstream logFile_;
    std::mutex mutex_;
    LogLevel logLevel_;
    std::string logPath_;

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
};

// Convenience macros with std::format support
#define LOG_DEBUG(fmt, ...)                                                    \
    do {                                                                       \
        Logger::getInstance().debug(std::format(fmt __VA_OPT__(,) __VA_ARGS__)); \
    } while (0)

#define LOG_INFO(fmt, ...)                                                     \
    do {                                                                       \
        Logger::getInstance().info(std::format(fmt __VA_OPT__(,) __VA_ARGS__)); \
    } while (0)

#define LOG_WARN(fmt, ...)                                                     \
    do {                                                                       \
        Logger::getInstance().warn(std::format(fmt __VA_OPT__(,) __VA_ARGS__)); \
    } while (0)

#define LOG_ERROR(fmt, ...)                                                    \
    do {                                                                       \
        Logger::getInstance().error(std::format(fmt __VA_OPT__(,) __VA_ARGS__)); \
    } while (0)

#endif // LOGGER_H
