/*
 * Audio Capture
 * Records audio from I2S microphone and encodes to WAV format
 */

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "../config.h"

class AudioCapture {
private:
    bool is_initialized;
    bool is_recording;
    uint8_t* audio_buffer;
    size_t buffer_size;
    size_t bytes_recorded;
    unsigned long record_start_time;
    
    void writeWavHeader(uint8_t* buffer, size_t data_size);
    
public:
    AudioCapture();
    ~AudioCapture();
    
    bool begin();
    void end();
    
    bool startRecording();
    bool stopRecording();
    bool isRecording() const { return is_recording; }
    
    // Get recorded audio data (includes WAV header)
    const uint8_t* getAudioData() const { return audio_buffer; }
    size_t getAudioSize() const { return bytes_recorded; }
    
    // Recording duration
    unsigned long getRecordingDuration() const;
    float getRecordingProgress() const;
};

#endif // AUDIO_CAPTURE_H
