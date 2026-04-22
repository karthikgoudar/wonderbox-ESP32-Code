#include "ProcessingAudioPlayer.h"

#include "../config.h"
#include "../utils/Logger.h"

ProcessingAudioPlayer::ProcessingAudioPlayer()
    : initialized(false), playing(false), data_start_offset(44) {
}

ProcessingAudioPlayer::~ProcessingAudioPlayer() {
    end();
}

bool ProcessingAudioPlayer::configureI2SOut() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = AUDIO_OUT_BCLK_PIN,
        .ws_io_num = AUDIO_OUT_LRCLK_PIN,
        .data_out_num = AUDIO_OUT_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(PROCESSING_AUDIO_I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Logger::error("ProcAudio", String("I2S TX install failed: ") + err);
        return false;
    }

    err = i2s_set_pin(PROCESSING_AUDIO_I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Logger::error("ProcAudio", String("I2S TX pin set failed: ") + err);
        i2s_driver_uninstall(PROCESSING_AUDIO_I2S_PORT);
        return false;
    }

    return true;
}

bool ProcessingAudioPlayer::validateAndSeekWavData(File& file, size_t& data_offset) {
    if (file.size() < 44) {
        Logger::error("ProcAudio", "Clip file too small for WAV header");
        return false;
    }

    uint8_t header[44];
    size_t read_size = file.read(header, sizeof(header));
    if (read_size != sizeof(header)) {
        Logger::error("ProcAudio", "Unable to read WAV header");
        return false;
    }

    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        Logger::error("ProcAudio", "Invalid WAV header");
        return false;
    }

    data_offset = 44;
    file.seek(data_offset);
    return true;
}

bool ProcessingAudioPlayer::openClipFile() {
    if (current_file) {
        current_file.close();
    }

    current_file = SPIFFS.open(PROCESSING_AUDIO_CLIP_PATH, "r");
    if (!current_file) {
        Logger::warning("ProcAudio", String("Clip file not found: ") + PROCESSING_AUDIO_CLIP_PATH);
        return false;
    }

    if (!validateAndSeekWavData(current_file, data_start_offset)) {
        current_file.close();
        return false;
    }

    return true;
}

bool ProcessingAudioPlayer::begin() {
    if (!ENABLE_PROCESSING_AUDIO) {
        initialized = false;
        return true;
    }

    if (!SPIFFS.begin(true)) {
        Logger::error("ProcAudio", "SPIFFS mount failed");
        return false;
    }

    if (!configureI2SOut()) {
        return false;
    }

    initialized = true;
    Logger::info("ProcAudio", "Processing audio player initialized");
    return true;
}

void ProcessingAudioPlayer::end() {
    stop();
    if (initialized) {
        i2s_driver_uninstall(PROCESSING_AUDIO_I2S_PORT);
    }
    initialized = false;
}

bool ProcessingAudioPlayer::startLoop() {
    if (!ENABLE_PROCESSING_AUDIO) {
        return true;
    }

    if (!initialized) {
        Logger::warning("ProcAudio", "Cannot start loop, player not initialized");
        return false;
    }

    if (!openClipFile()) {
        return false;
    }

    playing = true;
    Logger::info("ProcAudio", "Started processing clip loop");
    return true;
}

void ProcessingAudioPlayer::stop() {
    if (!playing) {
        return;
    }

    playing = false;
    if (current_file) {
        current_file.close();
    }
    i2s_zero_dma_buffer(PROCESSING_AUDIO_I2S_PORT);
    Logger::info("ProcAudio", "Stopped processing clip loop");
}

void ProcessingAudioPlayer::update() {
    if (!playing || !current_file) {
        return;
    }

    static uint8_t buffer[PROCESSING_AUDIO_CHUNK_SIZE];
    size_t read_size = current_file.read(buffer, sizeof(buffer));

    if (read_size == 0) {
        // Loop clip.
        current_file.seek(data_start_offset);
        read_size = current_file.read(buffer, sizeof(buffer));
        if (read_size == 0) {
            Logger::warning("ProcAudio", "Clip has no audio payload");
            stop();
            return;
        }
    }

    size_t bytes_written = 0;
    i2s_write(PROCESSING_AUDIO_I2S_PORT, buffer, read_size, &bytes_written, 10 / portTICK_PERIOD_MS);
}
