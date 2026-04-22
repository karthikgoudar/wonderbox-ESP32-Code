/*
 * State Machine Header
 * Manages the device state flow for sticker generation
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "../models/JobStatus.h"
#include "../utils/ErrorHandler.h"

enum class DeviceState {
    IDLE,
    RECORDING,
    UPLOADING,
    PROCESSING,
    DOWNLOADING,
    RENDERING,
    COMPLETE,
    ERROR,
    SLEEP
};

class StateMachine {
private:
    DeviceState current_state;
    DeviceState previous_state;
    unsigned long state_enter_time;
    unsigned long last_activity_time;
    ErrorHandler error_handler;
    JobStatus job_status;

    void enterState(DeviceState new_state);
    void handleStateTimeout();
    unsigned long getStateTimeout(DeviceState state) const;

public:
    StateMachine();
    
    void begin();
    void update();
    
    // State transitions
    void onButtonPressed();
    void onRecordingComplete(const uint8_t* audio_data, size_t size);
    void onUploadComplete(const String& job_id);
    void onProgressUpdate(ProcessingStep step);
    void onImageReady(const String& image_url);
    void onJobComplete(const String& sticker_id);
    void onError(ErrorCode code, const String& message);
    
    // State queries
    DeviceState getCurrentState() const { return current_state; }
    bool isIdle() const { return current_state == DeviceState::IDLE; }
    bool isProcessing() const { 
        return current_state != DeviceState::IDLE && 
               current_state != DeviceState::COMPLETE &&
               current_state != DeviceState::ERROR;
    }
    
    // Job status
    const JobStatus& getJobStatus() const { return job_status; }
    void resetJob();
    
    // Error handling
    bool shouldRetry() const { return error_handler.shouldRetry(); }
    void recordRetryAttempt() { error_handler.recordAttempt(); }
    unsigned long getRetryDelay() const { return error_handler.getBackoffDelay(); }
    
    // Timeout management
    bool hasStateTimedOut() const;
    unsigned long getTimeInState() const;
    unsigned long getTimeSinceActivity() const;
    bool shouldSleep() const;
};

#endif // STATE_MACHINE_H
