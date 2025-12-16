#include <iostream>
#include <glib.h>
#include "ibus_engine.h"
#include "logger.h"

int main(int argc, char *argv[]) {
    // Set locale
    setlocale(LC_ALL, "");

    LOG_INFO("=== Starting IBus LibIME Engine ===");
    LOG_INFO("Process ID: " << getpid());

    // Initialize IBus (g_type_init is deprecated and no longer needed in
    // GLib 2.36+)
    IBusEngineWrapper::init();

    std::cout << "IBus LibIME engine started (logs: /tmp/ibus-libime.log)"
              << std::endl;
    LOG_INFO("IBus initialized, entering main loop");

    // Run main loop
    IBusEngineWrapper::run();

    LOG_INFO("Main loop exited");
    return 0;
}
