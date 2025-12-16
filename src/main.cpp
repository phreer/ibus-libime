#include <format>
#include <iostream>
#include <glib.h>
#include "ibus_engine.h"
#include "logger.h"

int main(int argc, char *argv[]) {
    // Set locale
    setlocale(LC_ALL, "");

    LOG_INFO("=== Starting IBus LibIME Engine ===");
    LOG_INFO("Process ID: {}", getpid());

    // Initialize IBus (g_type_init is deprecated and no longer needed in
    // GLib 2.36+)
    IBusEngineWrapper::init();

    std::string logPath = Logger::getInstance().getLogPath();
    std::string logLevelStr;
    switch (Logger::getInstance().getLogLevel()) {
        case LogLevel::DEBUG: logLevelStr = "DEBUG"; break;
        case LogLevel::INFO: logLevelStr = "INFO"; break;
        case LogLevel::WARN: logLevelStr = "WARN"; break;
        case LogLevel::ERROR: logLevelStr = "ERROR"; break;
    }
    
    std::cout << std::format("IBus LibIME engine started\n"
                             "  Log file: {}\n"
                             "  Log level: {} (set IBUS_LIBIME_LOG_LEVEL to change)\n",
                             logPath, logLevelStr);
    
    LOG_INFO("IBus initialized, entering main loop");

    // Run main loop
    IBusEngineWrapper::run();

    LOG_INFO("Main loop exited");
    return 0;
}
