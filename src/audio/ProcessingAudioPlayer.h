/*
 * Processing Audio Player
 * Loops a PCM WAV clip from SPIFFS while backend generation is running.
 */

#ifndef PROCESSING_AUDIO_PLAYER_H
#define PROCESSING_AUDIO_PLAYER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <driver/i2s.h>

class ProcessingAudioPlayer {
private:
    bool initialized;
    bool playing;
    File current_file;
    size_t data_start_offset;

    bool configureI2SOut();
    bool openClipFile();
    bool validateAndSeekWavData(File& file, size_t& data_offset);

public:
    ProcessingAudioPlayer();
    ~ProcessingAudioPlayer();

    bool begin();
    void end();

    bool startLoop();
    void stop();
    void update();

    bool isPlaying() const { return playing; }
};

#endif // PROCESSING_AUDIO_PLAYER_H
