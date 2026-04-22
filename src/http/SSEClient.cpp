/*
 * SSE Client Implementation
 */

#include "SSEClient.h"
#include "../utils/Logger.h"
#include <ArduinoJson.h>

SSEClient::SSEClient() : is_streaming(false), stream(nullptr) {
}

void SSEClient::begin(const String& url) {
    base_url = url;
    Logger::info("SSEClient", "Initialized");
}

bool SSEClient::connect(const String& job_id_param) {
    job_id = job_id_param;
    
    char endpoint[128];
    snprintf(endpoint, sizeof(endpoint), ENDPOINT_STREAM, job_id.c_str());
    String url = base_url + String(endpoint);
    
    Logger::info("SSEClient", "Connecting to SSE stream: " + url);
    
    http.begin(url);
    http.addHeader("Accept", "text/event-stream");
    http.addHeader("Cache-Control", "no-cache");
    http.setTimeout(PROCESSING_TIMEOUT_MS);
    
    int http_code = http.GET();
    
    if (http_code == HTTP_CODE_OK) {
        stream = http.getStreamPtr();
        is_streaming = true;
        read_buffer = "";
        Logger::info("SSEClient", "SSE stream connected");
        return true;
    } else {
        Logger::error("SSEClient", String("SSE connection failed: HTTP ") + http_code);
        http.end();
        return false;
    }
}

void SSEClient::update() {
    if (!is_streaming || !stream) {
        return;
    }
    
    while (stream->available()) {
        char c = stream->read();
        
        if (c == '\n') {
            processLine(read_buffer);
            read_buffer = "";
        } else if (c != '\r') {
            read_buffer += c;
        }
    }
}

void SSEClient::processLine(const String& line) {
    if (line.length() == 0) {
        // Empty line signals end of event
        return;
    }
    
    if (line.startsWith("event:")) {
        String event_type = line.substring(6);
        event_type.trim();
        
        // Wait for data line
        while (!stream->available()) {
            delay(1);
        }
        
        String data_line = "";
        while (stream->available()) {
            char c = stream->read();
            if (c == '\n') break;
            if (c != '\r') data_line += c;
        }
        
        if (data_line.startsWith("data:")) {
            String data = data_line.substring(5);
            data.trim();
            
            SSEEvent event;
            event.event_type = event_type;
            event.data = data;
            event.timestamp = millis();
            
            Logger::debug("SSEClient", "Event: " + event_type + ", Data: " + data);
            
            if (event_callback) {
                event_callback(event);
            }
            
            // Close stream on terminal events
            if (event_type == "done" || event_type == "error") {
                Logger::info("SSEClient", "Terminal event received, closing stream");
                close();
            }
        }
    }
}

SSEEvent SSEClient::parseEvent(const String& event_line, const String& data_line) {
    SSEEvent event;
    event.timestamp = millis();
    
    if (event_line.startsWith("event:")) {
        event.event_type = event_line.substring(6);
        event.event_type.trim();
    }
    
    if (data_line.startsWith("data:")) {
        event.data = data_line.substring(5);
        event.data.trim();
    }
    
    return event;
}

void SSEClient::close() {
    if (is_streaming) {
        http.end();
        is_streaming = false;
        stream = nullptr;
        Logger::info("SSEClient", "Stream closed");
    }
}
