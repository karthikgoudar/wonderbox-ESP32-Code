/*
 * Recording Screen
 * Shows recording progress with visual feedback
 */

#ifndef RECORDING_SCREEN_H
#define RECORDING_SCREEN_H

#include "ScreenBase.h"

class RecordingScreen : public ScreenBase {
private:
    float progress;
    int recording_duration_sec;
    bool show_countdown;
    
public:
    RecordingScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), progress(0.0), recording_duration_sec(5), show_countdown(true) {
        use_partial_update = true;  // Use partial updates for smooth progress
    }
    
    void setProgress(float prog) { 
        progress = constrain(prog, 0.0, 1.0); 
    }
    
    void setDuration(int seconds) { recording_duration_sec = seconds; }
    void setShowCountdown(bool show) { show_countdown = show; }
    
    void render() override {
        display->setPartialWindow(150, 180, 500, 160);
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            
            // Title
            display->setTextSize(3);
            drawCenteredText("Listening...", 200);
            
            // Countdown or percentage
            if (show_countdown) {
                int remaining = (int)((1.0 - progress) * recording_duration_sec);
                display->setTextSize(4);
                drawCenteredText(String(remaining) + "s", 240);
            } else {
                display->setTextSize(2);
                drawCenteredText(String((int)(progress * 100)) + "%", 240);
            }
            
            // Progress bar
            drawProgressBar(200, 280, 400, 30, progress);
            
            // Instruction
            display->setTextSize(1);
            drawCenteredText("Speak clearly into the device", 320);
            
        } while (display->nextPage());
    }
    
    void update() override {
        // Called periodically to refresh display with new progress
        render();
    }
};

#endif // RECORDING_SCREEN_H
