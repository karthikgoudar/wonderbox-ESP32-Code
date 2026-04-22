/*
 * Processing Screen
 * Shows backend processing status
 */

#ifndef PROCESSING_SCREEN_H
#define PROCESSING_SCREEN_H

#include "ScreenBase.h"

class ProcessingScreen : public ScreenBase {
private:
    String current_step;
    String step_display_name;
    bool show_spinner;
    int spinner_frame;
    
    const char* spinner_chars[4] = {"|", "/", "-", "\\"};
    
    String getStepDisplayName(const String& step) {
        if (step == "transcribing") return "Listening...";
        if (step == "translating") return "Understanding...";
        if (step == "normalizing_prompt") return "Preparing...";
        if (step == "generating_image") return "Creating magic...";
        if (step == "processing_image") return "Adding details...";
        if (step == "uploading_to_storage") return "Saving...";
        return "Processing...";
    }
    
public:
    ProcessingScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), current_step(""), show_spinner(true), spinner_frame(0) {
        use_partial_update = true;
    }
    
    void setStep(const String& step) { 
        current_step = step;
        step_display_name = getStepDisplayName(step);
    }
    
    void setShowSpinner(bool show) { show_spinner = show; }
    
    void render() override {
        display->setPartialWindow(150, 200, 500, 100);
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            
            // Status text
            display->setTextSize(2);
            
            String status_text = step_display_name;
            if (show_spinner) {
                status_text += " " + String(spinner_chars[spinner_frame % 4]);
            }
            
            drawCenteredText(status_text, 230);
            
            // Sub-text
            display->setTextSize(1);
            drawCenteredText("This may take a moment", 260);
            
        } while (display->nextPage());
    }
    
    void update() override {
        spinner_frame++;
        render();
    }
    
    void onEnter() override {
        spinner_frame = 0;
    }
};

#endif // PROCESSING_SCREEN_H
