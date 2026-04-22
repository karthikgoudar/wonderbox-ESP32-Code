/*
 * WonderBox ESP32 Main Application
 * 
 * Coordinates all components to provide end-to-end sticker generation:
 * 1. Audio capture from I2S microphone
 * 2. Upload to cloud API
 * 3. Stream processing updates via SSE
 * 4. Download and display generated sticker image
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"
#include "utils/Logger.h"
#include "utils/ErrorHandler.h"
#include "models/JobStatus.h"
#include "state_machine/StateMachine.h"
#include "audio/AudioCapture.h"
#include "audio/ProcessingAudioPlayer.h"
#include "http/HttpClient.h"
#include "http/SSEClient.h"
#include "display/DisplayManager.h"
#include "print/PrinterManager.h"

// Global instances
StateMachine state_machine;
AudioCapture audio_capture;
ProcessingAudioPlayer processing_audio_player;
WonderBoxHttpClient http_client;
SSEClient sse_client;
DisplayManager display;
PrinterManager printer_manager;

// Hardware state
volatile bool button_pressed = false;
bool wifi_connected = false;

// Function declarations
void setupWiFi();
void setupHardware();
void IRAM_ATTR buttonISR();
void handleButtonPress();
void handleSSEEvent(const SSEEvent& event);
void performStateMachine();
void enterDeepSleep();
void handleAndReturnToWelcomeOnError(const String& error_message, bool recoverable = true);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Logger::info("Main", "=== WonderBox ESP32 Starting ===");
    Logger::printMemoryStats();
    
    // Initialize hardware
    setupHardware();
    
    // Initialize display
    if (!display.begin()) {
        Logger::error("Main", "Display initialization failed");
        ESP.restart();
    }
    
    // Connect to WiFi
    setupWiFi();
    
    // Initialize audio capture
    if (!audio_capture.begin()) {
        Logger::error("Main", "Audio capture initialization failed");
        handleAndReturnToWelcomeOnError("Audio init failed", false);
    }

    if (!processing_audio_player.begin()) {
        Logger::warning("Main", "Processing audio player unavailable; continuing without clip");
    }
    
    // Initialize HTTP client
    http_client.begin(String(API_BASE_URL), String(DEVICE_TOKEN), String(CHILD_ID));
    
    // Initialize SSE client
    sse_client.begin(String(API_BASE_URL));
    sse_client.setEventCallback(handleSSEEvent);
    
    // Initialize state machine
    state_machine.begin();

    if (!printer_manager.begin()) {
        Logger::warning("Main", "Printer manager initialization failed; print simulation disabled");
    }
    
    Logger::info("Main", "Setup complete");
    display.showWelcome();
}

void loop() {
    // Handle button press
    if (button_pressed) {
        button_pressed = false;
        handleButtonPress();
    }
    
    // Update SSE client
    if (sse_client.isStreaming()) {
        sse_client.update();
    }

    // Keep looping processing clip while active.
    processing_audio_player.update();

    // Keep active screen animations updated.
    display.update();
    
    // Update state machine
    state_machine.update();
    
    // Perform state-specific actions
    performStateMachine();
    
    // Check for deep sleep
    if (state_machine.shouldSleep()) {
        enterDeepSleep();
    }
    
    yield();
}

void handleAndReturnToWelcomeOnError(const String& error_message, bool recoverable) {
    processing_audio_player.stop();
    Logger::error("Main", "Error flow: " + error_message);
    display.showError(error_message, recoverable);
    delay(2000);
    state_machine.resetJob();
    display.showWelcome();
}

void setupWiFi() {
    Logger::info("WiFi", "Connecting to: " + String(WIFI_SSID));
    display.showProcessing("Connecting WiFi");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            Logger::error("WiFi", "Connection timeout");
            handleAndReturnToWelcomeOnError("WiFi timeout", false);
            return;
        }
        delay(500);
        Serial.print(".");
    }
    
    wifi_connected = true;
    Logger::info("WiFi", "Connected! IP: " + WiFi.localIP().toString());
}

void setupHardware() {
    // Button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    
    // LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Logger::info("Hardware", "Initialized");
}

void IRAM_ATTR buttonISR() {
    button_pressed = true;
}

void handleButtonPress() {
    Logger::info("Main", "Button pressed");
    digitalWrite(LED_PIN, HIGH);
    state_machine.onButtonPressed();
    delay(200);  // Debounce
    digitalWrite(LED_PIN, LOW);
}

void handleSSEEvent(const SSEEvent& event) {
    Logger::debug("Main", "SSE Event: " + event.event_type);
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (error) {
        Logger::error("Main", "JSON parse error: " + String(error.c_str()));
        return;
    }
    
    if (event.event_type == "status") {
        String state = doc["state"];
        Logger::info("Main", "Job state: " + state);
        
    } else if (event.event_type == "progress") {
        String step = doc["step"];
        Logger::info("Main", "Progress: " + step);
        display.showProcessing(step);
        
        // Map step string to enum
        if (step == "transcribing") {
            state_machine.onProgressUpdate(ProcessingStep::TRANSCRIBING);
        } else if (step == "generating_image") {
            state_machine.onProgressUpdate(ProcessingStep::GENERATING_IMAGE);
        }
        
    } else if (event.event_type == "text") {
        String text = doc["text"];
        String language = doc["language"];
        Logger::info("Main", "Transcript: " + text + " (" + language + ")");
        
    } else if (event.event_type == "image_ready") {
        String image_url = doc["image_url"];
        Logger::info("Main", "Image ready: " + image_url);
        processing_audio_player.stop();
        state_machine.onImageReady(image_url);
        
    } else if (event.event_type == "done") {
        String sticker_id = doc["sticker_id"];
        Logger::info("Main", "Job complete! Sticker ID: " + sticker_id);
        processing_audio_player.stop();
        state_machine.onJobComplete(sticker_id);
        
    } else if (event.event_type == "error") {
        String error_msg = doc["message"];
        Logger::error("Main", "Job error: " + error_msg);
        processing_audio_player.stop();
        state_machine.onError(ErrorCode::UNKNOWN_ERROR, error_msg);
        handleAndReturnToWelcomeOnError(error_msg, true);
    }
}

void performStateMachine() {
    DeviceState current = state_machine.getCurrentState();
    
    switch (current) {
        case DeviceState::IDLE:
            // Waiting for button press
            break;
            
        case DeviceState::RECORDING: {
            Logger::info("Main", "Starting audio recording");
            display.showRecording(0.0);
            
            if (audio_capture.startRecording()) {
                float progress = 0.0;
                while (audio_capture.isRecording()) {
                    progress = audio_capture.getRecordingProgress();
                    display.showRecording(progress);
                    delay(100);
                }
                
                state_machine.onRecordingComplete(
                    audio_capture.getAudioData(),
                    audio_capture.getAudioSize()
                );
            } else {
                state_machine.onError(ErrorCode::AUDIO_CAPTURE_FAILED, "Recording failed");
            }
            break;
        }
            
        case DeviceState::UPLOADING: {
            Logger::info("Main", "Uploading audio");
            display.showProcessing("Uploading");
            
            SubmitResponse response = http_client.submitAudio(
                audio_capture.getAudioData(),
                audio_capture.getAudioSize()
            );
            
            if (response.success) {
                state_machine.onUploadComplete(response.job_id);

                if (ENABLE_PROCESSING_AUDIO) {
                    processing_audio_player.startLoop();
                }
                
                // Connect to SSE stream
                if (sse_client.connect(response.job_id)) {
                    Logger::info("Main", "SSE stream connected");
                } else {
                    state_machine.onError(ErrorCode::HTTP_REQUEST_FAILED, "SSE connection failed");
                    handleAndReturnToWelcomeOnError("SSE connection failed", true);
                }
            } else {
                state_machine.onError(ErrorCode::HTTP_REQUEST_FAILED, response.error_message);
                handleAndReturnToWelcomeOnError(response.error_message, true);
            }
            break;
        }
            
        case DeviceState::PROCESSING:
            // SSE events handled by callback
            digitalWrite(LED_PIN, (millis() / 500) % 2);  // Blink LED
            break;
            
        case DeviceState::DOWNLOADING: {
            const JobStatus& job = state_machine.getJobStatus();
            Logger::info("Main", "Downloading image");
            display.showProcessing("Downloading");
            
            uint8_t* image_data = nullptr;
            size_t image_size = 0;
            
            if (http_client.downloadImage(job.image_url, &image_data, &image_size)) {
                Logger::info("Main", "Image downloaded successfully");

                // Render image first so child sees result immediately.
                display.showImage(image_data, image_size);

                if (ENABLE_PRINT_PIPELINE) {
                    display.showPrinting("Preparing print job", 0.2f);
                    String print_error;
                    bool printed = printer_manager.printImage(image_data, image_size, print_error);
                    if (!printed) {
                        Logger::warning("Main", "Print pipeline warning: " + print_error);
                        display.showPrinting("Printer issue", 1.0f);
                    } else {
                        display.showPrinting("Print complete", 1.0f);
                    }
                    delay(400);

                    // Return to image screen after print status.
                    display.showImage(image_data, image_size);
                }

                free(image_data);
                
                state_machine.onJobComplete(job.sticker_id);
            } else {
                state_machine.onError(ErrorCode::IMAGE_DOWNLOAD_FAILED, "Download failed");
                handleAndReturnToWelcomeOnError("Image download failed", true);
            }
            break;
        }
            
        case DeviceState::COMPLETE:
            Logger::info("Main", "Process complete!");
            digitalWrite(LED_PIN, HIGH);
            delay(2000);
            digitalWrite(LED_PIN, LOW);
            
            // Return to idle after delay
            delay(3000);
            state_machine.resetJob();
            display.showWelcome();
            break;
            
        case DeviceState::ERROR:
            processing_audio_player.stop();
            digitalWrite(LED_PIN, HIGH);
            delay(5000);  // Show error for 5 seconds
            digitalWrite(LED_PIN, LOW);
            
            // Return to idle
            state_machine.resetJob();
            display.showWelcome();
            break;
            
        case DeviceState::SLEEP:
            // Already handled in main loop
            break;
            
        default:
            break;
    }
}

void enterDeepSleep() {
    Logger::info("Main", "Entering deep sleep");
    
    display.sleep();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // Configure wake on button press
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);  // Wake on LOW
    
    Logger::info("Main", "Good night!");
    esp_deep_sleep_start();
}
