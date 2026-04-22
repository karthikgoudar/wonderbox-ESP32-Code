/*
 * Screen Base Class
 * Abstract base class for all display screens
 */

#ifndef SCREEN_BASE_H
#define SCREEN_BASE_H

#include <Arduino.h>
#include <GxEPD2_BW.h>

class ScreenBase {
protected:
    GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* display;
    bool use_partial_update;
    
public:
    ScreenBase(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : display(disp), use_partial_update(false) {}
    
    virtual ~ScreenBase() {}
    
    // Must be implemented by derived classes
    virtual void render() = 0;
    
    // Optional lifecycle methods
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update() {}
    
    // Helper methods for common operations
    void setPartialUpdate(bool partial) { use_partial_update = partial; }
    bool isPartialUpdate() const { return use_partial_update; }
    
    // Drawing helpers
    void drawCenteredText(const String& text, int16_t y, uint8_t size = 2) {
        display->setTextSize(size);
        int16_t x1, y1;
        uint16_t w, h;
        display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        int16_t x = (display->width() - w) / 2;
        display->setCursor(x, y);
        display->print(text);
    }
    
    void drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, float progress) {
        // Draw outline
        display->drawRect(x, y, width, height, GxEPD_BLACK);
        
        // Fill progress
        int16_t fill_width = (int16_t)(width * progress);
        if (fill_width > 0) {
            display->fillRect(x, y, fill_width, height, GxEPD_BLACK);
        }
    }
};

#endif // SCREEN_BASE_H
