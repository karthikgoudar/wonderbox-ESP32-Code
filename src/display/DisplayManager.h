/*
 * Display Manager
 * Coordinates display hardware and screen transitions
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "../config.h"
#include "screens/ScreenBase.h"
#include "screens/WelcomeScreen.h"
#include "screens/BootScreen.h"
#include "screens/RecordingScreen.h"
#include "screens/ProcessingScreen.h"
#include "screens/ImageScreen.h"
#include "screens/ErrorScreen.h"
#include "screens/PrintingScreen.h"

enum class ScreenType {
    BOOT,
    WELCOME,
    RECORDING,
    PROCESSING,
    PRINTING,
    IMAGE,
    ERROR
};

class DisplayManager {
private:
    GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* display;
    bool is_initialized;
    
    // Screen instances
    BootScreen* boot_screen;
    WelcomeScreen* welcome_screen;
    RecordingScreen* recording_screen;
    ProcessingScreen* processing_screen;
    PrintingScreen* printing_screen;
    ImageScreen* image_screen;
    ErrorScreen* error_screen;
    
    ScreenBase* current_screen;
    ScreenType current_screen_type;
    
    void switchToScreen(ScreenBase* screen, ScreenType type);
    
public:
    DisplayManager();
    ~DisplayManager();
    
    bool begin();
    void end();
    
    // Screen transitions
    void showBootAnimation();
    void showWelcome();
    void showRecording(float progress);
    void showProcessing(const String& step);
    void showPrinting(const String& status, float progress);
    void showImage(const uint8_t* image_data, size_t image_size);
    void showError(const String& error_message, bool recoverable = true);
    
    // Update current screen (for animations, progress, etc.)
    void update();
    
    // Direct access to screens for customization
    BootScreen* getBootScreen() { return boot_screen; }
    WelcomeScreen* getWelcomeScreen() { return welcome_screen; }
    RecordingScreen* getRecordingScreen() { return recording_screen; }
    ProcessingScreen* getProcessingScreen() { return processing_screen; }
    PrintingScreen* getPrintingScreen() { return printing_screen; }
    ImageScreen* getImageScreen() { return image_screen; }
    ErrorScreen* getErrorScreen() { return error_screen; }
    
    // Utility
    void clear();
    ScreenType getCurrentScreenType() const { return current_screen_type; }
    
    // Power management
    void sleep();
    void wake();
};

#endif // DISPLAY_MANAGER_H
