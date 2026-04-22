#include "PrinterManager.h"

#include "../config.h"
#include "../utils/Logger.h"

static HardwareSerial g_printerSerial(PRINTER_UART_PORT);
PrinterManager* PrinterManager::active_instance = nullptr;

PrinterManager::PrinterManager()
    : initialized(false),
      connected(false),
      printer_serial(&g_printerSerial),
      raster_buffer(nullptr),
      raster_width(0),
      raster_height(0),
      raster_row_bytes(0),
      source_width(0),
      source_height(0),
      decode_scale(1.0f),
      png_line_buffer(nullptr) {
}

PrinterManager::~PrinterManager() {
    freeRasterBuffer();
}

bool PrinterManager::beginThermalPrinter() {
#if ENABLE_THERMAL_PRINTER
    if (!printer_serial) {
        Logger::error("Printer", "Printer serial handle missing");
        return false;
    }

    printer_serial->begin(PRINTER_BAUD_RATE, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
    delay(100);
    writeEscPosInit();

    connected = true;
    Logger::info("Printer", "Thermal printer UART initialized");
    return true;
#else
    connected = false;
    return true;
#endif
}

void PrinterManager::writeEscPosInit() {
#if ENABLE_THERMAL_PRINTER
    const uint8_t init_cmd[] = {0x1B, 0x40};
    const uint8_t center_cmd[] = {0x1B, 0x61, 0x01};
    printer_serial->write(init_cmd, sizeof(init_cmd));
    printer_serial->write(center_cmd, sizeof(center_cmd));
#endif
}

void PrinterManager::writeEscPosLineFeed(uint8_t lines) {
#if ENABLE_THERMAL_PRINTER
    for (uint8_t index = 0; index < lines; index++) {
        printer_serial->write(0x0A);
    }
#endif
}

void PrinterManager::freeRasterBuffer() {
    if (raster_buffer) {
        free(raster_buffer);
        raster_buffer = nullptr;
    }

    if (png_line_buffer) {
        free(png_line_buffer);
        png_line_buffer = nullptr;
    }

    raster_width = 0;
    raster_height = 0;
    raster_row_bytes = 0;
    source_width = 0;
    source_height = 0;
    decode_scale = 1.0f;
}

bool PrinterManager::allocateRasterBuffer(uint16_t width, uint16_t height) {
    freeRasterBuffer();

    raster_width = width;
    raster_height = height;
    raster_row_bytes = (raster_width + 7) / 8;

    size_t raster_size = static_cast<size_t>(raster_row_bytes) * raster_height;
    raster_buffer = static_cast<uint8_t*>(calloc(raster_size, sizeof(uint8_t)));
    if (!raster_buffer) {
        Logger::error("Printer", "Failed to allocate printer raster buffer");
        return false;
    }

    return true;
}

void PrinterManager::plotScaledPixel(uint16_t src_x, uint16_t src_y, uint16_t rgb565) {
    if (!raster_buffer || source_width == 0 || source_height == 0 || decode_scale <= 0.0f) {
        return;
    }

    uint16_t dst_x = min<uint16_t>(raster_width - 1, static_cast<uint16_t>(src_x * decode_scale));
    uint16_t dst_y = min<uint16_t>(raster_height - 1, static_cast<uint16_t>(src_y * decode_scale));

    uint8_t red = ((rgb565 >> 11) & 0x1F) * 255 / 31;
    uint8_t green = ((rgb565 >> 5) & 0x3F) * 255 / 63;
    uint8_t blue = (rgb565 & 0x1F) * 255 / 31;
    uint16_t luma = static_cast<uint16_t>((red * 30U) + (green * 59U) + (blue * 11U)) / 100U;

    if (luma < PRINTER_LUMA_THRESHOLD) {
        size_t byte_index = static_cast<size_t>(dst_y) * raster_row_bytes + (dst_x / 8);
        raster_buffer[byte_index] |= (0x80 >> (dst_x % 8));
    }
}

void PrinterManager::pngDrawCallback(PNGDRAW* draw) {
    if (!active_instance || !draw) {
        return;
    }

    PrinterManager* self = active_instance;
    if (!self->png_line_buffer) {
        return;
    }

    self->png_decoder.getLineAsRGB565(draw, self->png_line_buffer, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFFFF);
    for (int x = 0; x < draw->iWidth; x++) {
        self->plotScaledPixel(static_cast<uint16_t>(x), static_cast<uint16_t>(draw->y), self->png_line_buffer[x]);
    }
}

int PrinterManager::jpegDrawCallback(JPEGDRAW* draw) {
    if (!active_instance || !draw) {
        return 0;
    }

    PrinterManager* self = active_instance;
    uint16_t* pixels = static_cast<uint16_t*>(draw->pPixels);
    for (int row = 0; row < draw->iHeight; row++) {
        for (int col = 0; col < draw->iWidth; col++) {
            uint16_t src_x = static_cast<uint16_t>(draw->x + col);
            uint16_t src_y = static_cast<uint16_t>(draw->y + row);
            uint16_t rgb565 = pixels[row * draw->iWidth + col];
            self->plotScaledPixel(src_x, src_y, rgb565);
        }
    }

    return 1;
}

bool PrinterManager::preparePngRaster(const uint8_t* image_data, size_t image_size, String& error_message) {
    int open_result = png_decoder.openRAM(const_cast<uint8_t*>(image_data), static_cast<int32_t>(image_size), pngDrawCallback);
    if (open_result != PNG_SUCCESS) {
        error_message = "PNG decode open failed";
        return false;
    }

    source_width = static_cast<uint16_t>(png_decoder.getWidth());
    source_height = static_cast<uint16_t>(png_decoder.getHeight());
    if (source_width == 0 || source_height == 0) {
        error_message = "Invalid PNG dimensions";
        png_decoder.close();
        return false;
    }

    decode_scale = min(1.0f, static_cast<float>(PRINTER_MAX_WIDTH_DOTS) / static_cast<float>(source_width));
    uint16_t target_width = max<uint16_t>(1, static_cast<uint16_t>(source_width * decode_scale));
    uint16_t target_height = max<uint16_t>(1, static_cast<uint16_t>(source_height * decode_scale));

    if (!allocateRasterBuffer(target_width, target_height)) {
        error_message = "Raster allocation failed";
        png_decoder.close();
        return false;
    }

    png_line_buffer = static_cast<uint16_t*>(malloc(static_cast<size_t>(source_width) * sizeof(uint16_t)));
    if (!png_line_buffer) {
        error_message = "PNG line buffer allocation failed";
        png_decoder.close();
        freeRasterBuffer();
        return false;
    }

    active_instance = this;
    int decode_result = png_decoder.decode(nullptr, 0);
    png_decoder.close();
    active_instance = nullptr;

    if (decode_result != PNG_SUCCESS) {
        error_message = "PNG decode failed";
        freeRasterBuffer();
        return false;
    }

    return true;
}

bool PrinterManager::prepareJpegRaster(const uint8_t* image_data, size_t image_size, String& error_message) {
    int open_result = jpeg_decoder.openRAM(const_cast<uint8_t*>(image_data), static_cast<int32_t>(image_size), jpegDrawCallback);
    if (open_result != JPEG_SUCCESS) {
        error_message = "JPEG decode open failed";
        return false;
    }

    source_width = static_cast<uint16_t>(jpeg_decoder.getWidth());
    source_height = static_cast<uint16_t>(jpeg_decoder.getHeight());
    if (source_width == 0 || source_height == 0) {
        error_message = "Invalid JPEG dimensions";
        jpeg_decoder.close();
        return false;
    }

    decode_scale = min(1.0f, static_cast<float>(PRINTER_MAX_WIDTH_DOTS) / static_cast<float>(source_width));
    uint16_t target_width = max<uint16_t>(1, static_cast<uint16_t>(source_width * decode_scale));
    uint16_t target_height = max<uint16_t>(1, static_cast<uint16_t>(source_height * decode_scale));

    if (!allocateRasterBuffer(target_width, target_height)) {
        error_message = "Raster allocation failed";
        jpeg_decoder.close();
        return false;
    }

    active_instance = this;
    int decode_result = jpeg_decoder.decode(0, 0, 0);
    jpeg_decoder.close();
    active_instance = nullptr;

    if (decode_result != JPEG_SUCCESS) {
        error_message = "JPEG decode failed";
        freeRasterBuffer();
        return false;
    }

    return true;
}

bool PrinterManager::prepareRasterFromImage(const uint8_t* image_data, size_t image_size, String& error_message) {
    if (image_size >= 8 && memcmp(image_data, "\x89PNG\r\n\x1A\n", 8) == 0) {
        Logger::info("Printer", "Detected PNG image for raster printing");
        return preparePngRaster(image_data, image_size, error_message);
    }

    if (image_size >= 3 && image_data[0] == 0xFF && image_data[1] == 0xD8 && image_data[2] == 0xFF) {
        Logger::info("Printer", "Detected JPEG image for raster printing");
        return prepareJpegRaster(image_data, image_size, error_message);
    }

    error_message = "Unsupported image format for printer";
    return false;
}

bool PrinterManager::streamRasterToPrinter(String& error_message) {
#if ENABLE_THERMAL_PRINTER
    if (!connected || !printer_serial) {
        error_message = "Thermal printer not connected";
        return false;
    }

    if (!raster_buffer || raster_width == 0 || raster_height == 0) {
        error_message = "Raster buffer is empty";
        return false;
    }

    writeEscPosInit();
    for (uint16_t start_row = 0; start_row < raster_height; start_row += PRINTER_RASTER_CHUNK_HEIGHT) {
        uint16_t chunk_height = min<uint16_t>(PRINTER_RASTER_CHUNK_HEIGHT, raster_height - start_row);
        uint8_t command[8];
        command[0] = 0x1D;
        command[1] = 0x76;
        command[2] = 0x30;
        command[3] = 0x00;
        command[4] = static_cast<uint8_t>(raster_row_bytes & 0xFF);
        command[5] = static_cast<uint8_t>((raster_row_bytes >> 8) & 0xFF);
        command[6] = static_cast<uint8_t>(chunk_height & 0xFF);
        command[7] = static_cast<uint8_t>((chunk_height >> 8) & 0xFF);
        printer_serial->write(command, sizeof(command));

        size_t chunk_offset = static_cast<size_t>(start_row) * raster_row_bytes;
        size_t chunk_size = static_cast<size_t>(chunk_height) * raster_row_bytes;
        printer_serial->write(raster_buffer + chunk_offset, chunk_size);
        printer_serial->flush();
    }

    writeEscPosLineFeed();
    return true;
#else
    Logger::info("Printer", "Simulated raster print complete");
    delay(PRINT_SIMULATED_DURATION_MS);
    return true;
#endif
}

bool PrinterManager::begin() {
    initialized = true;
    if (!beginThermalPrinter()) {
        initialized = false;
        return false;
    }

    Logger::info("Printer", "Printer manager initialized");
    return true;
}

bool PrinterManager::printImage(const uint8_t* image_data, size_t image_size, String& error_message) {
    if (!initialized) {
        error_message = "Printer manager not initialized";
        return false;
    }

    if (!image_data || image_size == 0) {
        error_message = "Invalid image buffer";
        return false;
    }

    if (!prepareRasterFromImage(image_data, image_size, error_message)) {
        return false;
    }

    Logger::info("Printer", String("Prepared raster ") + raster_width + "x" + raster_height);

    bool printed = streamRasterToPrinter(error_message);
    freeRasterBuffer();

    if (!printed) {
        return false;
    }

    Logger::info("Printer", "Print task complete");
    return true;
}
