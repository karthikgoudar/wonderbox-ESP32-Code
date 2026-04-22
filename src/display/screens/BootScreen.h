/*
 * Boot Screen
 * Simple frame-based boot animation for e-paper startup.
 */

#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include "ScreenBase.h"

class BootScreen : public ScreenBase {
private:
    uint8_t frame_index;
    uint8_t frame_count;

public:
    BootScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp)
        : ScreenBase(disp), frame_index(0), frame_count(4) {
        use_partial_update = false;
    }

    void setFrame(uint8_t frame) {
        frame_index = frame % frame_count;
    }

    void render() override {
        display->setFullWindow();
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);

            display->setTextSize(3);
            drawCenteredText("WonderBox", 150);

            display->setTextSize(2);
            drawCenteredText("Booting...", 200);

            // Draw a coarse frame-based progress animation.
            int16_t bar_x = 160;
            int16_t bar_y = 250;
            int16_t bar_w = 480;
            int16_t bar_h = 28;
            display->drawRect(bar_x, bar_y, bar_w, bar_h, GxEPD_BLACK);

            int16_t fill_w = (bar_w - 4) * (frame_index + 1) / frame_count;
            display->fillRect(bar_x + 2, bar_y + 2, fill_w, bar_h - 4, GxEPD_BLACK);

            display->setTextSize(1);
            drawCenteredText("Initializing display, audio, network", 320);
        } while (display->nextPage());
    }
};

#endif // BOOT_SCREEN_H
