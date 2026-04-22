/*
 * Welcome Screen
 * Shows initial "ready" message
 */

#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include "ScreenBase.h"

class WelcomeScreen : public ScreenBase {
private:
    String device_name;
    bool show_instructions;
    
public:
    WelcomeScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), device_name("WonderBox"), show_instructions(true) {}
    
    void setDeviceName(const String& name) { device_name = name; }
    void setShowInstructions(bool show) { show_instructions = show; }
    
    void render() override {
        display->setFullWindow();
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            
            // Title
            display->setTextSize(4);
            drawCenteredText(device_name, 200);
            
            // Subtitle
            display->setTextSize(3);
            drawCenteredText("Ready!", 250);
            
            // Instructions
            if (show_instructions) {
                display->setTextSize(2);
                drawCenteredText("Press button to start", 320);
            }
            
            // Optional: Draw decorative elements
            // drawLogo() or drawBorder()
            
        } while (display->nextPage());
    }
    
    void onEnter() override {
        // Could add entry animation or sound
    }
};

#endif // WELCOME_SCREEN_H
