# Display Screen Architecture

## Overview

The display system uses a **modular screen-based architecture** where each screen is a self-contained class responsible for its own rendering logic and behavior.

## Architecture

```
DisplayManager (Hardware Coordinator)
    ├── Manages e-paper hardware
    ├── Switches between screens
    └── Delegates rendering to screen classes

Screen Classes (Individual Screens)
    ├── ScreenBase (Abstract Base)
    ├── WelcomeScreen
    ├── RecordingScreen
    ├── ProcessingScreen
    ├── ImageScreen
    └── ErrorScreen
```

## Screen Lifecycle

Each screen has lifecycle hooks:

1. **onEnter()** - Called when switching TO this screen
2. **render()** - Main rendering method (must be implemented)
3. **update()** - Called periodically for animations/progress
4. **onExit()** - Called when switching AWAY from this screen

## Usage Examples

### Basic Usage

```cpp
DisplayManager display;

// Initialize
display.begin();

// Show different screens
display.showWelcome();
display.showRecording(0.5);  // 50% progress
display.showProcessing("transcribing");
display.showImage(image_data, image_size);
display.showError("Something went wrong", true);
```

### Customizing Screens

```cpp
// Access individual screens for customization
WelcomeScreen* welcome = display.getWelcomeScreen();
welcome->setDeviceName("My WonderBox");
welcome->setShowInstructions(false);

RecordingScreen* recording = display.getRecordingScreen();
recording->setDuration(3);  // 3 seconds
recording->setShowCountdown(true);

ErrorScreen* error = display.getErrorScreen();
error->setShowRetryButton(false);
```

### Animation Loop

```cpp
void loop() {
    // Update current screen for animations
    display.update();
    delay(100);
}
```

## Creating New Screens

### Step 1: Create Screen Class

```cpp
// src/display/screens/MyCustomScreen.h
#ifndef MY_CUSTOM_SCREEN_H
#define MY_CUSTOM_SCREEN_H

#include "ScreenBase.h"

class MyCustomScreen : public ScreenBase {
private:
    String message;
    
public:
    MyCustomScreen(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>* disp) 
        : ScreenBase(disp), message("") {}
    
    void setMessage(const String& msg) { message = msg; }
    
    void render() override {
        display->setFullWindow();
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            drawCenteredText(message, 240);
        } while (display->nextPage());
    }
};

#endif
```

### Step 2: Add to DisplayManager

```cpp
// DisplayManager.h
#include "screens/MyCustomScreen.h"

class DisplayManager {
private:
    MyCustomScreen* my_custom_screen;
    
public:
    void showCustom(const String& message);
    MyCustomScreen* getCustomScreen() { return my_custom_screen; }
};
```

```cpp
// DisplayManager.cpp
bool DisplayManager::begin() {
    // ... existing code ...
    my_custom_screen = new MyCustomScreen(display);
    // ...
}

void DisplayManager::showCustom(const String& message) {
    my_custom_screen->setMessage(message);
    switchToScreen(my_custom_screen, ScreenType::CUSTOM);
}
```

## Screen Features

### WelcomeScreen
- Customizable device name
- Optional instructions
- Full screen update

### RecordingScreen
- Real-time progress bar
- Countdown timer or percentage
- Partial updates for smooth animation

### ProcessingScreen
- Dynamic step names
- Animated spinner
- User-friendly step descriptions

### ImageScreen
- Full-screen image display
- Success message overlay
- Placeholder for image decoding (PNG/JPEG)

### ErrorScreen
- Error icon
- Error message with wrapping
- Recoverable vs. fatal errors
- Retry instructions

## Helper Methods (from ScreenBase)

```cpp
// Center text horizontally
drawCenteredText("Hello", 200, 2);  // text, y-position, size

// Draw progress bar
drawProgressBar(100, 250, 600, 30, 0.75);  // x, y, width, height, progress
```

## Partial vs. Full Updates

```cpp
class MyScreen : public ScreenBase {
public:
    MyScreen(GxEPD2_BW<...>* disp) : ScreenBase(disp) {
        use_partial_update = true;  // Fast updates
        // or
        use_partial_update = false; // Full refresh (better quality)
    }
};
```

**Partial Updates:**
- ✅ Faster (100-300ms)
- ✅ Good for animations/progress
- ❌ May show ghosting
- 💡 Use for: RecordingScreen, ProcessingScreen

**Full Updates:**
- ✅ Better quality
- ✅ No ghosting
- ❌ Slower (1-2 seconds)
- 💡 Use for: WelcomeScreen, ImageScreen, ErrorScreen

## Benefits of This Architecture

### 1. Single Responsibility
Each screen handles only its own logic:
- WelcomeScreen = welcome logic
- RecordingScreen = recording UI
- No mixing of concerns

### 2. Testability
```cpp
// Easy to test individual screens
WelcomeScreen screen(&mock_display);
screen.setDeviceName("Test");
screen.render();
// Verify output
```

### 3. Reusability
Screens can be reused in different contexts:
```cpp
// Use error screen for different errors
display.showError("WiFi failed", true);
display.showError("API error", true);
display.showError("Hardware fault", false);
```

### 4. Easy Extension
Add new screens without modifying existing ones:
- Create WiFiSetupScreen
- Create SettingsScreen
- Create DiagnosticsScreen

### 5. Customization
Users can customize screens independently:
```cpp
welcome->setDeviceName("Kid's WonderBox");
recording->setDuration(10);
error->setShowRetryButton(false);
```

## Performance Considerations

### Memory Usage
Each screen instance: ~50-100 bytes
Total screens (6): ~300-600 bytes
DisplayManager: ~200 bytes
**Total overhead: < 1KB**

### Rendering Speed
- Full update: 1-2 seconds
- Partial update: 100-300ms
- Screen switch: Instant (just pointer change)

## Future Enhancements

### Animations
```cpp
class AnimatedScreen : public ScreenBase {
    int frame;
    
    void update() override {
        frame++;
        render();  // Re-render with new frame
    }
};
```

### Transitions
```cpp
void DisplayManager::switchToScreen(ScreenBase* screen, ScreenType type) {
    current_screen->fadeOut();  // Transition effect
    current_screen = screen;
    screen->fadeIn();
}
```

### Touch Support
```cpp
class ScreenBase {
    virtual void onTouch(int16_t x, int16_t y) {}
};
```

## Comparison: Before vs. After

### Before (Monolithic)
```cpp
class DisplayManager {
    void showWelcomeScreen();
    void showRecordingIndicator(float);
    void showProcessingStatus(String);
    void showImage(uint8_t*, size_t);
    void showError(String);
    // All rendering logic mixed together
};
```

### After (Modular)
```cpp
class DisplayManager {
    WelcomeScreen* welcome_screen;
    RecordingScreen* recording_screen;
    // ... other screens
    
    void showWelcome();
    void showRecording(float);
    // Simple delegation
};

// Each screen is independent
class WelcomeScreen : public ScreenBase {
    void render() { /* welcome logic */ }
};
```

## Summary

The screen-based architecture provides:
- ✅ Better organization (one file per screen)
- ✅ Easier testing (test screens independently)
- ✅ Simpler maintenance (modify one screen at a time)
- ✅ Greater flexibility (customize each screen)
- ✅ Cleaner code (smaller, focused classes)
- ✅ Easy extension (add screens without touching existing code)

Perfect for growing UIs with multiple states and interactions!
