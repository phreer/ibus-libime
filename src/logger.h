#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
public:
    static Logger &getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string &level, const std::string &message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_ << getCurrentTime() << " [" << level << "] " << message
                     << std::endl;
            logFile_.flush();
        }
    }

    void debug(const std::string &message) { log("DEBUG", message); }
    void info(const std::string &message) { log("INFO", message); }
    void warn(const std::string &message) { log("WARN", message); }
    void error(const std::string &message) { log("ERROR", message); }

private:
    Logger() {
        logFile_.open("/tmp/ibus-libime.log", std::ios::app);
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

    std::string getCurrentTime() {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    std::ofstream logFile_;
    std::mutex mutex_;

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
};

// Convenience macros
#define LOG_DEBUG(msg)                                                         \
    do {                                                                       \
        std::ostringstream oss;                                                \
        oss << msg;                                                            \
        Logger::getInstance().debug(oss.str());                                \
    } while (0)

#define LOG_INFO(msg)                                                          \
    do {                                                                       \
        std::ostringstream oss;                                                \
        oss << msg;                                                            \
        Logger::getInstance().info(oss.str());                                 \
    } while (0)

#define LOG_WARN(msg)                                                          \
    do {                                                                       \
        std::ostringstream oss;                                                \
        oss << msg;                                                            \
        Logger::getInstance().warn(oss.str());                                 \
    } while (0)

#define LOG_ERROR(msg)                                                         \
    do {                                                                       \
        std::ostringstream oss;                                                \
        oss << msg;                                                            \
        Logger::getInstance().error(oss.str());                                \
    } while (0)

#endif // LOGGER_H
