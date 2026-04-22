/*
 * SSE (Server-Sent Events) Client
 * Streams real-time progress updates from the backend
 */

#ifndef SSE_CLIENT_H
#define SSE_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include "../models/ApiResponse.h"
#include "../models/JobStatus.h"
#include "../config.h"

typedef std::function<void(const SSEEvent&)> SSEEventCallback;

class SSEClient {
private:
    HTTPClient http;
    WiFiClient* stream;
    String base_url;
    String job_id;
    bool is_streaming;
    SSEEventCallback event_callback;
    
    String read_buffer;
    
    SSEEvent parseEvent(const String& event_line, const String& data_line);
    void processLine(const String& line);

public:
    SSEClient();
    
    void begin(const String& base_url);
    bool connect(const String& job_id);
    void update();
    void close();
    
    void setEventCallback(SSEEventCallback callback) {
        event_callback = callback;
    }
    
    bool isStreaming() const { return is_streaming; }
};

#endif // SSE_CLIENT_H
