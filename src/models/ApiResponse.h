/*
 * API Response Models
 */

#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include <Arduino.h>

struct SubmitResponse {
    bool success;
    String job_id;
    String error_message;
    int http_code;

    SubmitResponse() : success(false), http_code(0) {}
};

struct SSEEvent {
    String event_type;  // "status", "progress", "text", "image_ready", "done", "error"
    String data;        // JSON data
    unsigned long timestamp;

    SSEEvent() : timestamp(0) {}
    
    bool isEmpty() const {
        return event_type.isEmpty() && data.isEmpty();
    }
};

#endif // API_RESPONSE_H
