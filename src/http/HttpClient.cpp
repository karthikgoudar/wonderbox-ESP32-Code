/*
 * HTTP Client Implementation
 */

#include "HttpClient.h"
#include <base64.h>

WonderBoxHttpClient::WonderBoxHttpClient() {
}

void WonderBoxHttpClient::begin(const String& url, const String& token, const String& child) {
    base_url = url;
    device_token = token;
    child_id = child;
    Logger::info("HttpClient", "Initialized with base URL: " + base_url);
}

void WonderBoxHttpClient::setCommonHeaders() {
    http.addHeader("x-device-token", device_token);
    http.addHeader("User-Agent", "WonderBox-ESP32/1.0");
}

bool WonderBoxHttpClient::checkMemory() {
    size_t free_heap = ESP.getFreeHeap();
    if (free_heap < MIN_FREE_HEAP) {
        Logger::error("HttpClient", String("Low memory: ") + free_heap + " bytes");
        return false;
    }
    return true;
}

SubmitResponse WonderBoxHttpClient::submitAudio(const uint8_t* audio_data, size_t audio_size) {
    SubmitResponse response;
    
    if (!checkMemory()) {
        response.error_message = "Insufficient memory";
        return response;
    }

    Logger::info("HttpClient", String("Submitting audio: ") + audio_size + " bytes");

    // Build multipart form data
    String boundary = "----WonderBoxBoundary" + String(random(0xffff), HEX);
    String url = base_url + String(ENDPOINT_SUBMIT);
    
    http.begin(url);
    setCommonHeaders();
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http.setTimeout(API_TIMEOUT_MS);

    // Build multipart body
    String header = "--" + boundary + "\r\n";
    header += "Content-Disposition: form-data; name=\"child_id\"\r\n\r\n";
    header += child_id + "\r\n";
    header += "--" + boundary + "\r\n";
    header += "Content-Disposition: form-data; name=\"audio\"; filename=\"audio.wav\"\r\n";
    header += "Content-Type: audio/wav\r\n\r\n";
    
    String footer = "\r\n--" + boundary + "--\r\n";
    
    // Calculate total size
    size_t total_size = header.length() + audio_size + footer.length();
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        Logger::error("HttpClient", "Failed to allocate buffer");
        response.error_message = "Memory allocation failed";
        http.end();
        return response;
    }
    
    // Build request body
    memcpy(buffer, header.c_str(), header.length());
    memcpy(buffer + header.length(), audio_data, audio_size);
    memcpy(buffer + header.length() + audio_size, footer.c_str(), footer.length());
    
    // Send POST request
    int http_code = http.POST(buffer, total_size);
    free(buffer);
    
    response.http_code = http_code;
    
    if (http_code == HTTP_CODE_OK || http_code == HTTP_CODE_CREATED) {
        String payload = http.getString();
        Logger::debug("HttpClient", "Response: " + payload);
        
        // Parse JSON response
        // Expected: {"job_id": "uuid"}
        int job_id_pos = payload.indexOf("\"job_id\"");
        if (job_id_pos > 0) {
            int start = payload.indexOf("\"", job_id_pos + 9) + 1;
            int end = payload.indexOf("\"", start);
            response.job_id = payload.substring(start, end);
            response.success = true;
            Logger::info("HttpClient", "Received job_id: " + response.job_id);
        } else {
            Logger::error("HttpClient", "No job_id in response");
            response.error_message = "Invalid server response";
        }
    } else {
        String error = "HTTP " + String(http_code);
        if (http_code > 0) {
            error += ": " + http.getString();
        } else {
            error += ": Connection failed";
        }
        Logger::error("HttpClient", error);
        response.error_message = error;
    }
    
    http.end();
    return response;
}

bool WonderBoxHttpClient::downloadImage(const String& image_url, uint8_t** image_data, size_t* image_size) {
    if (!checkMemory()) {
        return false;
    }

    Logger::info("HttpClient", "Downloading image: " + image_url);
    
    http.begin(image_url);
    http.setTimeout(API_TIMEOUT_MS);
    
    int http_code = http.GET();
    
    if (http_code == HTTP_CODE_OK) {
        int len = http.getSize();
        
        if (len <= 0 || len > MAX_IMAGE_FILE_SIZE) {
            Logger::error("HttpClient", String("Invalid image size: ") + len);
            http.end();
            return false;
        }
        
        *image_data = (uint8_t*)malloc(len);
        if (!*image_data) {
            Logger::error("HttpClient", "Failed to allocate image buffer");
            http.end();
            return false;
        }
        
        WiFiClient* stream = http.getStreamPtr();
        size_t bytes_read = 0;
        uint8_t buffer[128];
        
        while (http.connected() && bytes_read < len) {
            size_t available = stream->available();
            if (available) {
                int c = stream->readBytes(buffer, min(available, sizeof(buffer)));
                memcpy(*image_data + bytes_read, buffer, c);
                bytes_read += c;
            }
            delay(1);
        }
        
        *image_size = bytes_read;
        Logger::info("HttpClient", String("Downloaded ") + bytes_read + " bytes");
        http.end();
        return true;
    } else {
        Logger::error("HttpClient", String("Download failed: HTTP ") + http_code);
        http.end();
        return false;
    }
}

bool WonderBoxHttpClient::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void WonderBoxHttpClient::end() {
    http.end();
}
