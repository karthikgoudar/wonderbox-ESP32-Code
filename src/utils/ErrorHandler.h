/*
 * Error Handler with Retry Logic
 * Implements exponential backoff retry mechanism
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "Logger.h"

enum class ErrorCode {
    NONE,
    WIFI_CONNECT_FAILED,
    WIFI_TIMEOUT,
    HTTP_REQUEST_FAILED,
    HTTP_TIMEOUT,
    AUDIO_CAPTURE_FAILED,
    AUDIO_BUFFER_OVERFLOW,
    IMAGE_DOWNLOAD_FAILED,
    IMAGE_RENDER_FAILED,
    MEMORY_ALLOCATION_FAILED,
    INVALID_RESPONSE,
    DEVICE_BUSY,
    LIMIT_EXCEEDED,
    UNKNOWN_ERROR
};

class ErrorHandler {
private:
    int retry_count;
    unsigned long last_retry_time;
    ErrorCode last_error;
    String last_error_message;

public:
    ErrorHandler() : retry_count(0), last_retry_time(0), last_error(ErrorCode::NONE) {}

    bool shouldRetry() const {
        return retry_count < MAX_RETRIES;
    }

    unsigned long getBackoffDelay() const {
        // Exponential backoff: base * multiplier^attempt
        unsigned long delay = RETRY_BACKOFF_BASE_MS;
        for (int i = 0; i < retry_count; i++) {
            delay *= RETRY_BACKOFF_MULTIPLIER;
        }
        return min(delay, (unsigned long)RETRY_BACKOFF_MAX_MS);
    }

    void recordAttempt() {
        retry_count++;
        last_retry_time = millis();
        Logger::debug("ErrorHandler", String("Retry attempt ") + retry_count + "/" + MAX_RETRIES);
    }

    void recordError(ErrorCode code, const String& message) {
        last_error = code;
        last_error_message = message;
        Logger::error("ErrorHandler", message);
    }

    void reset() {
        retry_count = 0;
        last_retry_time = 0;
        last_error = ErrorCode::NONE;
        last_error_message = "";
    }

    int getRetryCount() const {
        return retry_count;
    }

    ErrorCode getLastError() const {
        return last_error;
    }

    String getLastErrorMessage() const {
        return last_error_message;
    }

    String getErrorCodeString(ErrorCode code) const {
        switch (code) {
            case ErrorCode::NONE: return "NONE";
            case ErrorCode::WIFI_CONNECT_FAILED: return "WIFI_CONNECT_FAILED";
            case ErrorCode::WIFI_TIMEOUT: return "WIFI_TIMEOUT";
            case ErrorCode::HTTP_REQUEST_FAILED: return "HTTP_REQUEST_FAILED";
            case ErrorCode::HTTP_TIMEOUT: return "HTTP_TIMEOUT";
            case ErrorCode::AUDIO_CAPTURE_FAILED: return "AUDIO_CAPTURE_FAILED";
            case ErrorCode::AUDIO_BUFFER_OVERFLOW: return "AUDIO_BUFFER_OVERFLOW";
            case ErrorCode::IMAGE_DOWNLOAD_FAILED: return "IMAGE_DOWNLOAD_FAILED";
            case ErrorCode::IMAGE_RENDER_FAILED: return "IMAGE_RENDER_FAILED";
            case ErrorCode::MEMORY_ALLOCATION_FAILED: return "MEMORY_ALLOCATION_FAILED";
            case ErrorCode::INVALID_RESPONSE: return "INVALID_RESPONSE";
            case ErrorCode::DEVICE_BUSY: return "DEVICE_BUSY";
            case ErrorCode::LIMIT_EXCEEDED: return "LIMIT_EXCEEDED";
            default: return "UNKNOWN_ERROR";
        }
    }

    bool isRecoverable(ErrorCode code) const {
        switch (code) {
            case ErrorCode::WIFI_TIMEOUT:
            case ErrorCode::HTTP_TIMEOUT:
            case ErrorCode::HTTP_REQUEST_FAILED:
                return true;
            case ErrorCode::LIMIT_EXCEEDED:
            case ErrorCode::DEVICE_BUSY:
            case ErrorCode::MEMORY_ALLOCATION_FAILED:
                return false;
            default:
                return AUTO_RETRY_ON_ERROR;
        }
    }
};

#endif // ERROR_HANDLER_H
