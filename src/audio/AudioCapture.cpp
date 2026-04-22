/*
 * Audio Capture Implementation
 */

#include "AudioCapture.h"
#include "../utils/Logger.h"

AudioCapture::AudioCapture() : 
    is_initialized(false),
    is_recording(false),
    audio_buffer(nullptr),
    buffer_size(0),
    bytes_recorded(0),
    record_start_time(0) {
}

AudioCapture::~AudioCapture() {
    end();
}

bool AudioCapture::begin() {
    if (is_initialized) {
        Logger::info("AudioCapture", "Already initialized");
        return true;
    }
    
    Logger::info("AudioCapture", "Initializing I2S microphone");
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = AUDIO_DMA_BUF_COUNT,
        .dma_buf_len = AUDIO_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };
    
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Logger::error("AudioCapture", String("I2S driver install failed: ") + err);
        return false;
    }
    
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Logger::error("AudioCapture", String("I2S pin config failed: ") + err);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }
    
    // Allocate audio buffer
    // WAV header (44 bytes) + audio data
    size_t data_size = AUDIO_SAMPLE_RATE * (AUDIO_BITS_PER_SAMPLE / 8) * 
                       AUDIO_CHANNELS * (AUDIO_RECORD_DURATION_MS / 1000);
    buffer_size = 44 + data_size;
    
    audio_buffer = (uint8_t*)malloc(buffer_size);
    if (!audio_buffer) {
        Logger::error("AudioCapture", "Failed to allocate audio buffer");
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }
    
    is_initialized = true;
    Logger::info("AudioCapture", String("I2S initialized. Buffer size: ") + buffer_size);
    return true;
}

void AudioCapture::end() {
    if (is_recording) {
        stopRecording();
    }
    
    if (audio_buffer) {
        free(audio_buffer);
        audio_buffer = nullptr;
    }
    
    if (is_initialized) {
        i2s_driver_uninstall(I2S_PORT);
        is_initialized = false;
    }
}

bool AudioCapture::startRecording() {
    if (!is_initialized || is_recording) {
        return false;
    }
    
    Logger::info("AudioCapture", "Starting recording");
    
    // Clear I2S DMA buffer
    i2s_zero_dma_buffer(I2S_PORT);
    
    bytes_recorded = 44;  // Reserve space for WAV header
    record_start_time = millis();
    is_recording = true;
    
    size_t bytes_read;
    unsigned long start_time = millis();
    
    while (is_recording && (millis() - start_time) < AUDIO_RECORD_DURATION_MS) {
        if (bytes_recorded >= buffer_size) {
            Logger::warning("AudioCapture", "Buffer full");
            break;
        }
        
        size_t bytes_to_read = min((size_t)AUDIO_BUFFER_SIZE, buffer_size - bytes_recorded);
        
        esp_err_t err = i2s_read(I2S_PORT, 
                                 audio_buffer + bytes_recorded, 
                                 bytes_to_read, 
                                 &bytes_read, 
                                 portMAX_DELAY);
        
        if (err == ESP_OK) {
            bytes_recorded += bytes_read;
        } else {
            Logger::error("AudioCapture", String("I2S read error: ") + err);
            is_recording = false;
            return false;
        }
        
        yield();
    }
    
    is_recording = false;
    
    // Write WAV header
    size_t data_size = bytes_recorded - 44;
    writeWavHeader(audio_buffer, data_size);
    
    Logger::info("AudioCapture", String("Recording complete: ") + bytes_recorded + " bytes");
    return true;
}

bool AudioCapture::stopRecording() {
    if (!is_recording) {
        return false;
    }
    
    is_recording = false;
    Logger::info("AudioCapture", "Recording stopped manually");
    return true;
}

unsigned long AudioCapture::getRecordingDuration() const {
    if (record_start_time == 0) {
        return 0;
    }
    return millis() - record_start_time;
}

float AudioCapture::getRecordingProgress() const {
    if (!is_recording) {
        return 1.0f;
    }
    return (float)getRecordingDuration() / (float)AUDIO_RECORD_DURATION_MS;
}

void AudioCapture::writeWavHeader(uint8_t* buffer, size_t data_size) {
    uint32_t file_size = data_size + 36;
    uint16_t audio_format = 1;  // PCM
    uint16_t num_channels = AUDIO_CHANNELS;
    uint32_t sample_rate = AUDIO_SAMPLE_RATE;
    uint16_t bits_per_sample = AUDIO_BITS_PER_SAMPLE;
    uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    uint16_t block_align = num_channels * bits_per_sample / 8;
    
    uint8_t* p = buffer;
    
    // RIFF chunk
    memcpy(p, "RIFF", 4); p += 4;
    *(uint32_t*)p = file_size; p += 4;
    memcpy(p, "WAVE", 4); p += 4;
    
    // fmt chunk
    memcpy(p, "fmt ", 4); p += 4;
    *(uint32_t*)p = 16; p += 4;  // Chunk size
    *(uint16_t*)p = audio_format; p += 2;
    *(uint16_t*)p = num_channels; p += 2;
    *(uint32_t*)p = sample_rate; p += 4;
    *(uint32_t*)p = byte_rate; p += 4;
    *(uint16_t*)p = block_align; p += 2;
    *(uint16_t*)p = bits_per_sample; p += 2;
    
    // data chunk
    memcpy(p, "data", 4); p += 4;
    *(uint32_t*)p = data_size; p += 4;
}
