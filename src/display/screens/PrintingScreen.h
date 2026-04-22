/*
 * Printing Screen
 * Shows printing/export progress after image is ready.
 */

#ifndef PRINTING_SCREEN_H
#define PRINTING_SCREEN_H

#include "ScreenBase.h"

class PrintingScreen : public ScreenBase {
private:
    String status_text;
    float progress;

public:
    PrintingScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp)
        : ScreenBase(disp), status_text("Preparing print"), progress(0.0f) {
        use_partial_update = true;
    }

    void setStatus(const String& text) {
        status_text = text;
    }

    void setProgress(float value) {
        progress = constrain(value, 0.0f, 1.0f);
    }

    void render() override {
        display->setPartialWindow(120, 160, 560, 220);
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            display->setTextSize(3);
            drawCenteredText("Printing...", 210);

            display->setTextSize(2);
            drawCenteredText(status_text, 260);

            drawProgressBar(160, 310, 480, 28, progress);
        } while (display->nextPage());
    }

    void update() override {
        render();
    }
};

#endif // PRINTING_SCREEN_H
