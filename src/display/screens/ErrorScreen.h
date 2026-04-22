/*
 * Error Screen
 * Displays error messages with retry instructions
 */

#ifndef ERROR_SCREEN_H
#define ERROR_SCREEN_H

#include "ScreenBase.h"

class ErrorScreen : public ScreenBase {
private:
    String error_message;
    String error_code;
    bool show_retry_button;
    bool is_recoverable;
    
    void drawErrorIcon(int16_t x, int16_t y, int16_t size) {
        // Draw a simple error icon (triangle with !)
        int16_t half = size / 2;
        
        // Triangle outline
        display->drawLine(x, y - half, x - half, y + half, GxEPD_BLACK);
        display->drawLine(x, y - half, x + half, y + half, GxEPD_BLACK);
        display->drawLine(x - half, y + half, x + half, y + half, GxEPD_BLACK);
        
        // Exclamation mark
        display->fillRect(x - 2, y - 20, 4, 25, GxEPD_BLACK);
        display->fillRect(x - 2, y + 10, 4, 4, GxEPD_BLACK);
    }
    
public:
    ErrorScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), error_message("Unknown error"), error_code(""),
          show_retry_button(true), is_recoverable(true) {
        use_partial_update = false;
    }
    
    void setError(const String& message, const String& code = "") {
        error_message = message;
        error_code = code;
    }
    
    void setRecoverable(bool recoverable) { 
        is_recoverable = recoverable;
        show_retry_button = recoverable;
    }
    
    void setShowRetryButton(bool show) { show_retry_button = show; }
    
    void render() override {
        display->setFullWindow();
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            
            // Error icon
            drawErrorIcon(display->width() / 2, 150, 60);
            
            // "Error" title
            display->setTextSize(3);
            drawCenteredText("Oops!", 220);
            
            // Error message (word wrap if needed)
            display->setTextSize(2);
            
            // Simple word wrapping
            String wrapped = error_message;
            if (wrapped.length() > 30) {
                wrapped = wrapped.substring(0, 27) + "...";
            }
            drawCenteredText(wrapped, 260);
            
            // Error code (if provided)
            if (error_code.length() > 0) {
                display->setTextSize(1);
                drawCenteredText("Code: " + error_code, 290);
            }
            
            // Retry instructions
            if (show_retry_button) {
                display->setTextSize(2);
                drawCenteredText("Press button to retry", 330);
            } else {
                display->setTextSize(1);
                drawCenteredText("Please contact support", 330);
            }
            
        } while (display->nextPage());
    }
    
    void onEnter() override {
        // Could log error to serial/memory for diagnostics
    }
};

#endif // ERROR_SCREEN_H
