/*
 * Job Status Model
 * Represents the state of a sticker generation job
 */

#ifndef JOB_STATUS_H
#define JOB_STATUS_H

#include <Arduino.h>

enum class JobState {
    UNKNOWN,
    QUEUED,
    PROCESSING,
    DONE,
    ERROR,
    CANCELLED
};

enum class ProcessingStep {
    NONE,
    STARTING,
    VALIDATING_DEVICE,
    VALIDATING_CHILD,
    CHECKING_LIMITS,
    TRANSCRIBING,
    TRANSLATING,
    NORMALIZING_PROMPT,
    GENERATING_IMAGE,
    PROCESSING_IMAGE,
    UPLOADING_TO_STORAGE,
    SAVING_TO_DB,
    COMPLETE
};

struct JobStatus {
    String job_id;
    JobState state;
    ProcessingStep current_step;
    String transcript_text;
    String language;
    String image_url;
    String sticker_id;
    String error_code;
    String error_message;
    unsigned long timestamp;

    JobStatus() : 
        state(JobState::UNKNOWN),
        current_step(ProcessingStep::NONE),
        language("en"),
        timestamp(0) {}
    
    bool isComplete() const {
        return state == JobState::DONE || 
               state == JobState::ERROR || 
               state == JobState::CANCELLED;
    }

    bool hasError() const {
        return state == JobState::ERROR;
    }

    String getStateString() const {
        switch (state) {
            case JobState::QUEUED: return "queued";
            case JobState::PROCESSING: return "processing";
            case JobState::DONE: return "done";
            case JobState::ERROR: return "error";
            case JobState::CANCELLED: return "cancelled";
            default: return "unknown";
        }
    }

    String getStepString() const {
        switch (current_step) {
            case ProcessingStep::STARTING: return "starting";
            case ProcessingStep::VALIDATING_DEVICE: return "validating_device";
            case ProcessingStep::VALIDATING_CHILD: return "validating_child";
            case ProcessingStep::CHECKING_LIMITS: return "checking_limits";
            case ProcessingStep::TRANSCRIBING: return "transcribing";
            case ProcessingStep::TRANSLATING: return "translating";
            case ProcessingStep::NORMALIZING_PROMPT: return "normalizing_prompt";
            case ProcessingStep::GENERATING_IMAGE: return "generating_image";
            case ProcessingStep::PROCESSING_IMAGE: return "processing_image";
            case ProcessingStep::UPLOADING_TO_STORAGE: return "uploading_to_storage";
            case ProcessingStep::SAVING_TO_DB: return "saving_to_db";
            case ProcessingStep::COMPLETE: return "complete";
            default: return "none";
        }
    }
};

#endif // JOB_STATUS_H
