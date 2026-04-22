/*
 * HTTP Client
 * Handles REST API communication with WonderBox backend
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "../models/ApiResponse.h"
#include "../utils/Logger.h"
#include "../config.h"

class WonderBoxHttpClient {
private:
    HTTPClient http;
    String base_url;
    String device_token;
    String child_id;

    void setCommonHeaders();
    bool checkMemory();

public:
    WonderBoxHttpClient();
    
    void begin(const String& base_url, const String& device_token, const String& child_id);
    
    // Submit audio for processing
    SubmitResponse submitAudio(const uint8_t* audio_data, size_t audio_size);
    
    // Download image from URL
    bool downloadImage(const String& image_url, uint8_t** image_data, size_t* image_size);
    
    // Utility
    bool isConnected() const;
    void end();
};

#endif // HTTP_CLIENT_H
