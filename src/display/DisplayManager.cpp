/*
 * Display Manager Implementation
 */

#include "DisplayManager.h"
#include "../utils/Logger.h"

DisplayManager::DisplayManager()
    : display(nullptr),
      is_initialized(false),
      boot_screen(nullptr),
      welcome_screen(nullptr),
      recording_screen(nullptr),
      processing_screen(nullptr),
      printing_screen(nullptr),
      image_screen(nullptr),
      error_screen(nullptr),
      current_screen(nullptr),
      current_screen_type(ScreenType::WELCOME) {
}

DisplayManager::~DisplayManager() {
    end();
}

bool DisplayManager::begin() {
    Logger::info("Display", "Initializing e-paper display");

#ifdef DISPLAY_TYPE_WAVESHARE_7_5_V2
    display = new GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>(
        GxEPD2_750_T7(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
    );
#else
    Logger::error("Display", "Unsupported display type");
    return false;
#endif

    if (!display) {
        Logger::error("Display", "Failed to create display object");
        return false;
    }

    display->init(115200);
    display->setRotation(DISPLAY_ROTATION);
    display->setTextColor(GxEPD_BLACK);

    boot_screen = new BootScreen(display);
    welcome_screen = new WelcomeScreen(display);
    recording_screen = new RecordingScreen(display);
    processing_screen = new ProcessingScreen(display);
    printing_screen = new PrintingScreen(display);
    image_screen = new ImageScreen(display);
    error_screen = new ErrorScreen(display);

    is_initialized = true;
    Logger::info("Display", "Display initialized with screen system");

    showBootAnimation();
    showWelcome();
    return true;
}

void DisplayManager::end() {
    delete boot_screen;
    delete welcome_screen;
    delete recording_screen;
    delete processing_screen;
    delete printing_screen;
    delete image_screen;
    delete error_screen;

    boot_screen = nullptr;
    welcome_screen = nullptr;
    recording_screen = nullptr;
    processing_screen = nullptr;
    printing_screen = nullptr;
    image_screen = nullptr;
    error_screen = nullptr;
    current_screen = nullptr;

    if (display) {
        display->hibernate();
        delete display;
        display = nullptr;
    }

    is_initialized = false;
}

void DisplayManager::switchToScreen(ScreenBase* screen, ScreenType type) {
    if (!is_initialized || !screen) {
        return;
    }

    if (current_screen) {
        current_screen->onExit();
    }

    current_screen = screen;
    current_screen_type = type;

    current_screen->onEnter();
    current_screen->render();
}

void DisplayManager::showBootAnimation() {
    if (!is_initialized || !boot_screen) {
        return;
    }

    Logger::debug("Display", "Showing boot animation");

    for (uint8_t frame = 0; frame < BOOT_ANIMATION_FRAME_COUNT; frame++) {
        boot_screen->setFrame(frame);
        switchToScreen(boot_screen, ScreenType::BOOT);
        delay(BOOT_ANIMATION_FRAME_DELAY_MS);
    }
}

void DisplayManager::showWelcome() {
    if (!is_initialized) {
        return;
    }

    Logger::debug("Display", "Showing welcome screen");
    switchToScreen(welcome_screen, ScreenType::WELCOME);
}

void DisplayManager::showRecording(float progress) {
    if (!is_initialized || !recording_screen) {
        return;
    }

    if (current_screen_type != ScreenType::RECORDING) {
        Logger::debug("Display", "Switching to recording screen");
        switchToScreen(recording_screen, ScreenType::RECORDING);
    }

    recording_screen->setProgress(progress);
    recording_screen->update();
}

void DisplayManager::showProcessing(const String& step) {
    if (!is_initialized || !processing_screen) {
        return;
    }

    if (current_screen_type != ScreenType::PROCESSING) {
        Logger::debug("Display", "Switching to processing screen");
        switchToScreen(processing_screen, ScreenType::PROCESSING);
    }

    processing_screen->setStep(step);
    processing_screen->render();
}

void DisplayManager::showPrinting(const String& status, float progress) {
    if (!is_initialized || !printing_screen) {
        return;
    }

    if (current_screen_type != ScreenType::PRINTING) {
        Logger::debug("Display", "Switching to printing screen");
        switchToScreen(printing_screen, ScreenType::PRINTING);
    }

    printing_screen->setStatus(status);
    printing_screen->setProgress(progress);
    printing_screen->update();
}

void DisplayManager::showImage(const uint8_t* image_data, size_t image_size) {
    if (!is_initialized || !image_screen || !image_data || image_size == 0) {
        return;
    }

    Logger::info("Display", String("Showing image: ") + image_size + " bytes");
    image_screen->setImage(image_data, image_size);
    switchToScreen(image_screen, ScreenType::IMAGE);
}

void DisplayManager::showError(const String& error_message, bool recoverable) {
    if (!is_initialized || !error_screen) {
        return;
    }

    Logger::error("Display", "Showing error: " + error_message);
    error_screen->setError(error_message);
    error_screen->setRecoverable(recoverable);
    switchToScreen(error_screen, ScreenType::ERROR);
}

void DisplayManager::update() {
    if (!is_initialized || !current_screen) {
        return;
    }

    current_screen->update();
}

void DisplayManager::clear() {
    if (!is_initialized || !display) {
        return;
    }

    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());

    current_screen = nullptr;
}

void DisplayManager::sleep() {
    if (!is_initialized || !display) {
        return;
    }

    Logger::debug("Display", "Entering sleep mode");
    display->hibernate();
}

void DisplayManager::wake() {
    if (!is_initialized || !display) {
        return;
    }

    Logger::debug("Display", "Waking from sleep");
    display->init();
}
