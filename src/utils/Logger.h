/*
 * Logger Utility
 * Provides debug logging with levels
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_VERBOSE 5

class Logger {
public:
    static void begin(unsigned long baud = SERIAL_BAUD_RATE) {
        Serial.begin(baud);
        delay(100);
        info("Logger", "Initialized");
    }

    static void error(const char* tag, const char* message) {
        if (LOG_LEVEL >= LOG_LEVEL_ERROR) {
            Serial.printf("[ERROR] [%s] %s\n", tag, message);
        }
    }

    static void error(const char* tag, const String& message) {
        error(tag, message.c_str());
    }

    static void warning(const char* tag, const char* message) {
        if (LOG_LEVEL >= LOG_LEVEL_WARNING) {
            Serial.printf("[WARN]  [%s] %s\n", tag, message);
        }
    }

    static void warning(const char* tag, const String& message) {
        warning(tag, message.c_str());
    }

    static void info(const char* tag, const char* message) {
        if (LOG_LEVEL >= LOG_LEVEL_INFO) {
            Serial.printf("[INFO]  [%s] %s\n", tag, message);
        }
    }

    static void info(const char* tag, const String& message) {
        info(tag, message.c_str());
    }

    static void debug(const char* tag, const char* message) {
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            Serial.printf("[DEBUG] [%s] %s\n", tag, message);
        }
    }

    static void debug(const char* tag, const String& message) {
        debug(tag, message.c_str());
    }

    static void verbose(const char* tag, const char* message) {
        if (LOG_LEVEL >= LOG_LEVEL_VERBOSE) {
            Serial.printf("[VERB]  [%s] %s\n", tag, message);
        }
    }

    static void verbose(const char* tag, const String& message) {
        verbose(tag, message.c_str());
    }

    static void printMemoryStats() {
        if (LOG_MEMORY_USAGE) {
            Serial.printf("[MEM] Free heap: %d bytes, Min free: %d bytes\n",
                ESP.getFreeHeap(), ESP.getMinFreeHeap());
        }
    }
};

#endif // LOGGER_H
