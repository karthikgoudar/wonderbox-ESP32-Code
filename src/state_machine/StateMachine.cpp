/*
 * State Machine Implementation
 */

#include "StateMachine.h"
#include "../config.h"
#include "../utils/Logger.h"

StateMachine::StateMachine() : 
    current_state(DeviceState::IDLE),
    previous_state(DeviceState::IDLE),
    state_enter_time(0),
    last_activity_time(0) {
}

void StateMachine::begin() {
    Logger::info("StateMachine", "Initialized");
    enterState(DeviceState::IDLE);
}

void StateMachine::enterState(DeviceState new_state) {
    if (new_state == current_state) {
        return;
    }

    previous_state = current_state;
    current_state = new_state;
    state_enter_time = millis();
    last_activity_time = millis();

    Logger::info("StateMachine", String("State transition: ") + 
                 static_cast<int>(previous_state) + " -> " + 
                 static_cast<int>(current_state));
}

void StateMachine::update() {
    // Check for state timeout
    if (hasStateTimedOut()) {
        handleStateTimeout();
    }

    // Check for sleep condition
    if (shouldSleep()) {
        Logger::info("StateMachine", "Entering sleep mode due to inactivity");
        enterState(DeviceState::SLEEP);
    }
}

void StateMachine::handleStateTimeout() {
    Logger::warning("StateMachine", String("State timeout in state: ") + 
                    static_cast<int>(current_state));
    
    switch (current_state) {
        case DeviceState::RECORDING:
            onError(ErrorCode::AUDIO_CAPTURE_FAILED, "Recording timeout");
            break;
        case DeviceState::UPLOADING:
            onError(ErrorCode::HTTP_TIMEOUT, "Upload timeout");
            break;
        case DeviceState::PROCESSING:
            onError(ErrorCode::HTTP_TIMEOUT, "Processing timeout");
            break;
        case DeviceState::DOWNLOADING:
            onError(ErrorCode::IMAGE_DOWNLOAD_FAILED, "Download timeout");
            break;
        case DeviceState::RENDERING:
            onError(ErrorCode::IMAGE_RENDER_FAILED, "Rendering timeout");
            break;
        default:
            break;
    }
}

unsigned long StateMachine::getStateTimeout(DeviceState state) const {
    switch (state) {
        case DeviceState::RECORDING:
            return RECORDING_TIMEOUT_MS;
        case DeviceState::UPLOADING:
            return UPLOADING_TIMEOUT_MS;
        case DeviceState::PROCESSING:
            return PROCESSING_TIMEOUT_MS;
        case DeviceState::DOWNLOADING:
            return DOWNLOADING_TIMEOUT_MS;
        case DeviceState::RENDERING:
            return RENDERING_TIMEOUT_MS;
        default:
            return 0;  // No timeout
    }
}

bool StateMachine::hasStateTimedOut() const {
    unsigned long timeout = getStateTimeout(current_state);
    if (timeout == 0) {
        return false;
    }
    return getTimeInState() > timeout;
}

unsigned long StateMachine::getTimeInState() const {
    return millis() - state_enter_time;
}

unsigned long StateMachine::getTimeSinceActivity() const {
    return millis() - last_activity_time;
}

bool StateMachine::shouldSleep() const {
    return current_state == DeviceState::IDLE && 
           ENABLE_DEEP_SLEEP && 
           getTimeSinceActivity() > IDLE_SLEEP_TIMEOUT_MS;
}

void StateMachine::onButtonPressed() {
    Logger::info("StateMachine", "Button pressed");
    last_activity_time = millis();

    if (current_state == DeviceState::IDLE || 
        current_state == DeviceState::COMPLETE ||
        current_state == DeviceState::ERROR ||
        current_state == DeviceState::SLEEP) {
        resetJob();
        error_handler.reset();
        enterState(DeviceState::RECORDING);
    }
}

void StateMachine::onRecordingComplete(const uint8_t* audio_data, size_t size) {
    Logger::info("StateMachine", String("Recording complete: ") + size + " bytes");
    last_activity_time = millis();

    if (current_state == DeviceState::RECORDING) {
        enterState(DeviceState::UPLOADING);
    }
}

void StateMachine::onUploadComplete(const String& job_id) {
    Logger::info("StateMachine", String("Upload complete. Job ID: ") + job_id);
    last_activity_time = millis();

    job_status.job_id = job_id;
    job_status.state = JobState::PROCESSING;
    
    if (current_state == DeviceState::UPLOADING) {
        enterState(DeviceState::PROCESSING);
    }
}

void StateMachine::onProgressUpdate(ProcessingStep step) {
    Logger::debug("StateMachine", String("Progress: ") + job_status.getStepString());
    last_activity_time = millis();

    job_status.current_step = step;
}

void StateMachine::onImageReady(const String& image_url) {
    Logger::info("StateMachine", String("Image ready: ") + image_url);
    last_activity_time = millis();

    job_status.image_url = image_url;
    
    if (current_state == DeviceState::PROCESSING) {
        enterState(DeviceState::DOWNLOADING);
    }
}

void StateMachine::onJobComplete(const String& sticker_id) {
    Logger::info("StateMachine", String("Job complete. Sticker ID: ") + sticker_id);
    last_activity_time = millis();

    job_status.sticker_id = sticker_id;
    job_status.state = JobState::DONE;
    
    enterState(DeviceState::COMPLETE);
}

void StateMachine::onError(ErrorCode code, const String& message) {
    error_handler.recordError(code, message);
    
    job_status.state = JobState::ERROR;
    job_status.error_code = error_handler.getErrorCodeString(code);
    job_status.error_message = message;
    
    if (error_handler.isRecoverable(code) && error_handler.shouldRetry()) {
        Logger::warning("StateMachine", String("Recoverable error detected; retry delay hint=") +
                       error_handler.getBackoffDelay() + "ms");
    } else {
        Logger::error("StateMachine", "Unrecoverable error or max retries reached");
    }

    // Always move to ERROR so UI can consistently reset to welcome.
    enterState(DeviceState::ERROR);
}

void StateMachine::resetJob() {
    job_status = JobStatus();
    Logger::debug("StateMachine", "Job reset");
}
