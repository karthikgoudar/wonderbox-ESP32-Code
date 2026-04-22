/*
 * Printer Manager
 * Placeholder print pipeline for future thermal or external printer support.
 */

#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <Arduino.h>
#include <PNGdec.h>
#include <JPEGDEC.h>

class HardwareSerial;
struct PNGDRAW;
struct JPEGDRAW;

class PrinterManager {
private:
    bool initialized;
    bool connected;
    HardwareSerial* printer_serial;
    PNG png_decoder;
    JPEGDEC jpeg_decoder;
    uint8_t* raster_buffer;
    uint16_t raster_width;
    uint16_t raster_height;
    uint16_t raster_row_bytes;
    uint16_t source_width;
    uint16_t source_height;
    float decode_scale;
    uint16_t* png_line_buffer;

    static PrinterManager* active_instance;

    bool beginThermalPrinter();
    void writeEscPosInit();
    void writeEscPosLineFeed(uint8_t lines = 3);
    void freeRasterBuffer();
    bool allocateRasterBuffer(uint16_t width, uint16_t height);
    bool prepareRasterFromImage(const uint8_t* image_data, size_t image_size, String& error_message);
    bool preparePngRaster(const uint8_t* image_data, size_t image_size, String& error_message);
    bool prepareJpegRaster(const uint8_t* image_data, size_t image_size, String& error_message);
    bool streamRasterToPrinter(String& error_message);
    void plotScaledPixel(uint16_t src_x, uint16_t src_y, uint16_t rgb565);
    static void pngDrawCallback(PNGDRAW* draw);
    static int jpegDrawCallback(JPEGDRAW* draw);

public:
    PrinterManager();
    ~PrinterManager();

    bool begin();
    bool isInitialized() const { return initialized; }
    bool isConnected() const { return connected; }

    // Prints through serial thermal printer if enabled. Otherwise simulates.
    bool printImage(const uint8_t* image_data, size_t image_size, String& error_message);
};

#endif // PRINTER_MANAGER_H
