/*
 * Image Screen
 * Displays the generated sticker image
 */

#ifndef IMAGE_SCREEN_H
#define IMAGE_SCREEN_H

#include "ScreenBase.h"

class ImageScreen : public ScreenBase {
private:
    const uint8_t* image_data;
    size_t image_size;
    bool show_success_message;
    unsigned long display_start_time;
    
    void drawImageCentered(const uint8_t* data, size_t size) {
        // Placeholder for actual image decoding and rendering
        // In production, use PNGDEC or TJpgDec library
        
        // For now, show success message
        display->setTextSize(3);
        drawCenteredText("Your sticker is ready!", 200);
        
        display->setTextSize(2);
        drawCenteredText("Size: " + String(size / 1024) + " KB", 250);
        
        // TODO: Implement actual image decoding and rendering
        // - Decode PNG/JPEG
        // - Resize/scale to fit display
        // - Convert to 1-bit (dithering)
        // - Draw centered
    }
    
public:
    ImageScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), image_data(nullptr), image_size(0), 
          show_success_message(true), display_start_time(0) {
        use_partial_update = false;  // Always full refresh for images
    }
    
    void setImage(const uint8_t* data, size_t size) {
        image_data = data;
        image_size = size;
    }
    
    void setShowSuccessMessage(bool show) { show_success_message = show; }
    
    void render() override {
        display->setFullWindow();
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            
            if (image_data && image_size > 0) {
                drawImageCentered(image_data, image_size);
            } else {
                // No image data
                display->setTextSize(2);
                drawCenteredText("No image available", 240);
            }
            
            // Optional success message overlay
            if (show_success_message) {
                display->setTextSize(1);
                display->setCursor(10, display->height() - 20);
                display->print("Press button for new sticker");
            }
            
        } while (display->nextPage());
    }
    
    void onEnter() override {
        display_start_time = millis();
    }
    
    unsigned long getDisplayDuration() const {
        return millis() - display_start_time;
    }
};

#endif // IMAGE_SCREEN_H
