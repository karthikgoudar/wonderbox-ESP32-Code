# Quick Start Guide

## Prerequisites

1. **Install PlatformIO**
   - VS Code: Install PlatformIO IDE extension
   - CLI: `pip install platformio`

2. **Hardware Required**
   - ESP32 development board (ESP32-DevKit or ESP32-WROVER)
   - INMP441 I2S microphone module
   - Waveshare 7.5" e-paper display (or compatible)
   - Push button
   - LED
   - Breadboard and jumper wires

## Setup Steps

### 1. Configure Project

Copy the configuration template:
```bash
cp src/config.h.example src/config.h
```

Edit `src/config.h` with your settings:
- WiFi credentials (SSID and password)
- API base URL (your WonderBox backend server)
- Device token (from backend device registration)
- Child ID (from backend)
- Verify hardware pin assignments match your wiring

### 2. Wire Hardware

Connect components according to pin definitions in config.h:

**I2S Microphone (INMP441):**
- WS (Word Select) → GPIO 25
- SCK (Serial Clock) → GPIO 26
- SD (Serial Data) → GPIO 27
- VDD → 3.3V
- GND → GND

**E-Paper Display (SPI):**
- CS → GPIO 15
- DC → GPIO 27
- RST → GPIO 26
- BUSY → GPIO 25
- MOSI → GPIO 23
- SCK → GPIO 18
- VCC → 3.3V
- GND → GND

**Button & LED:**
- Button → GPIO 0 (with pull-up)
- LED → GPIO 2 (through resistor)

*Note: Adjust pins in config.h if needed to avoid conflicts*

### 3. Build and Upload

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

Or use VS Code PlatformIO buttons:
- ✓ Build
- → Upload
- 🔌 Serial Monitor

### 4. Test

1. Power on device
2. Wait for "WonderBox Ready!" on display
3. Press button to start recording
4. Speak for 3-5 seconds (e.g., "a cute dragon")
5. Watch progress on display:
   - Uploading...
   - Processing: transcribing
   - Processing: generating_image
   - Downloading...
6. Generated sticker appears on display!

## Troubleshooting

### WiFi won't connect
- Check SSID/password in config.h
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength

### Audio not recording
- Verify I2S pin connections
- Check microphone power (3.3V)
- Monitor serial output for I2S errors

### API errors
- Verify API_BASE_URL is correct
- Check DEVICE_TOKEN is valid
- Ensure CHILD_ID exists in backend
- Test backend with curl/Postman first

### Display issues
- Check SPI connections
- Verify display type in config.h
- Try lower SPI speed

### Out of memory
- Reduce AUDIO_RECORD_DURATION_MS
- Reduce AUDIO_DMA_BUF_COUNT
- Use ESP32-WROVER (more PSRAM)

## Configuration Options

Key settings in `config.h`:

- `AUDIO_RECORD_DURATION_MS` - Recording length (default 5000ms)
- `AUDIO_SAMPLE_RATE` - Sample rate (default 16000 Hz)
- `API_TIMEOUT_MS` - HTTP request timeout
- `MAX_RETRIES` - Number of retry attempts on error
- `ENABLE_DEEP_SLEEP` - Power saving mode
- `LOG_LEVEL` - Debug verbosity

## Next Steps

1. **Customize prompts**: Update age-based context prefixes in backend
2. **Adjust recording**: Tune audio duration and quality
3. **Power optimization**: Enable deep sleep, adjust timeouts
4. **Hardware case**: Design enclosure for child-friendly device
5. **Battery power**: Add LiPo battery and charging circuit

## Support

For issues:
1. Check serial monitor output (LOG_LEVEL_DEBUG)
2. Review hardware connections
3. Test backend API independently
4. Check memory stats (Logger::printMemoryStats())

Enjoy creating wonderful coloring book stickers! 🎨
