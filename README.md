# WonderBox ESP32 Firmware

ESP32 firmware for the WonderBox IoT device - an interactive sticker generation device for children.

## Features

✅ **Audio Capture** - Records child's voice using I2S microphone  
✅ **WiFi Connectivity** - Connects to WonderBox backend API  
✅ **HTTP Client** - Efficient REST API communication  
✅ **SSE Streaming** - Real-time progress updates via Server-Sent Events  
✅ **State Machine** - Robust state management with error handling  
✅ **Modular Screen System** - Clean screen-based UI architecture (see [SCREEN_ARCHITECTURE.md](docs/SCREEN_ARCHITECTURE.md))  
✅ **Image Rendering** - Displays generated coloring stickers on e-paper display  
✅ **Retry Logic** - Automatic retry with exponential backoff  
✅ **OTA Updates** - Over-the-air firmware updates  
✅ **Power Management** - Deep sleep and power optimization  

## Hardware Requirements

- **ESP32-WROVER or ESP32-WROOM** module
- **I2S Microphone** (e.g., INMP441, SPH0645)
- **E-Paper Display** (e.g., Waveshare 7.5" or 4.2")
- **Status LED** (for visual feedback)
- **Button** (for triggering recording)
- **Power supply** (3.3V regulated)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        ESP32 Device                          │
│                                                               │
│  ┌──────────┐   ┌────────────┐   ┌─────────────┐           │
│  │  Button  │──>│   State    │<──│ LED/Display │           │
│  │ Handler  │   │  Machine   │   │   Handler   │           │
│  └──────────┘   └─────┬──────┘   └─────────────┘           │
│                       │                                       │
│         ┌─────────────┼─────────────┐                        │
│         │             │             │                         │
│   ┌─────▼────┐  ┌────▼────┐  ┌────▼────┐                   │
│   │  Audio   │  │  HTTP   │  │  Image  │                    │
│   │ Capture  │  │ Client  │  │ Renderer│                    │
│   └──────────┘  └────┬────┘  └─────────┘                   │
│                      │                                        │
└──────────────────────┼────────────────────────────────────┘
                       │
                       ▼
              ┌────────────────┐
              │  WonderBox API  │
              │   (Backend)     │
              └────────────────┘
```

## State Machine

```
    IDLE
     │
     ├──(button press)──> RECORDING
     │                        │
     │                        ├──(audio captured)──> UPLOADING
     │                        │                          │
     │                        │                          ├──> PROCESSING
     │                        │                          │      │
     │<────────(error)────────┘                          │      ├──> DOWNLOADING
     │                                                    │      │      │
     │<────────────────(error)────────────────────────────┘      │      ├──> RENDERING
     │                                                            │      │      │
     │<───────────────────────────(error)──────────────────────────┘      │
     │                                                                     │
     └<─────────────────────────────────────(success)─────────────────────┘
```

## API Integration

### 1. Submit Audio
```cpp
POST /sticker/submit
Headers:
  - x-device-token: <device_token>
  - Content-Type: multipart/form-data
Body:
  - child_id: <child_id>
  - audio: <audio_file>
Response:
  { "job_id": "uuid" }
```

### 2. Stream Progress
```cpp
GET /sticker/{job_id}/stream
Headers:
  - Accept: text/event-stream
Response: SSE stream
  event: status
  data: {"state": "processing"}
  
  event: progress
  data: {"step": "transcribing"}
  
  event: text
  data: {"text": "a flying dragon", "language": "en"}
  
  event: image_ready
  data: {"image_url": "https://..."}
  
  event: done
  data: {"sticker_id": "uuid"}
```

## Quick Start

### 1. Install PlatformIO

```bash
# Using VS Code
# Install PlatformIO IDE extension

# Or using CLI
pip install platformio
```

### 2. Clone and Configure

```bash
git clone <repository>
cd wonder-box-esp32

# Copy configuration template
cp src/config.h.example src/config.h

# Edit config.h with your settings
```

### 3. Configure Device

Edit `src/config.h`:

```cpp
// WiFi Configuration
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// Backend API Configuration
#define API_BASE_URL "https://api.wonderbox.com"
#define DEVICE_TOKEN "your-device-token-here"
#define CHILD_ID "your-child-id"

// Hardware Pins (adjust based on your wiring)
#define I2S_WS_PIN 25
#define I2S_SCK_PIN 26
#define I2S_SD_PIN 27
#define BUTTON_PIN 4
#define LED_PIN 2
```

### 4. Build and Upload

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
wonder-box-esp32/
├── platformio.ini          # PlatformIO configuration
├── README.md              # This file
├── LICENSE                # MIT License
├── docs/                  # Documentation
│   ├── HARDWARE.md       # Hardware wiring guide
│   ├── API.md            # API integration details
│   └── TROUBLESHOOTING.md
├── src/
│   ├── main.cpp           # Entry point
│   ├── config.h.example   # Configuration template
│   ├── state_machine/     # State management
│   │   ├── StateMachine.h
│   │   └── StateMachine.cpp
│   ├── audio/             # Audio capture
│   │   ├── AudioCapture.h
│   │   └── AudioCapture.cpp
│   ├── http/              # HTTP client and SSE
│   │   ├── HttpClient.h
│   │   ├── HttpClient.cpp
│   │   ├── SSEClient.h
│   │   └── SSEClient.cpp
│   ├── display/           # Display management
│   │   ├── DisplayManager.h
│   │   ├── DisplayManager.cpp
│   │   └── screens/       # Modular screen classes
│   │       ├── ScreenBase.h
│   │       ├── WelcomeScreen.h
│   │       ├── RecordingScreen.h
│   │       ├── ProcessingScreen.h
│   │       ├── ImageScreen.h
│   │       └── ErrorScreen.h
│   ├── utils/             # Utilities
│   │   ├── ErrorHandler.h
│   │   ├── ErrorHandler.cpp
│   │   ├── Logger.h
│   │   └── Logger.cpp
│   └── models/            # Data structures
│       ├── JobStatus.h
│       └── ApiResponse.h
├── lib/                   # Custom libraries
└── test/                  # Unit tests
```

## Configuration

### WiFi Modes

- **Station Mode**: Connect to existing WiFi network
- **AP Mode**: Create access point for configuration
- **SmartConfig**: ESP-Touch configuration via mobile app

### Power Management

- **Active Mode**: Full power during operation
- **Light Sleep**: Between operations
- **Deep Sleep**: Long idle periods (wakes on button press)

### Error Handling

- **Retry Logic**: Exponential backoff (100ms, 200ms, 400ms...)
- **Max Retries**: Configurable per operation type
- **Fallback**: Graceful degradation and user feedback

## Development

### Serial Debugging

```cpp
// Enable debug logging in config.h
#define DEBUG_MODE true
#define LOG_LEVEL LOG_LEVEL_DEBUG

// Monitor output
pio device monitor --baud 115200
```

### Testing

```bash
# Run unit tests
pio test

# Run specific test
pio test -f test_state_machine
```

### OTA Updates

```cpp
// Enable OTA in config.h
#define ENABLE_OTA true
#define OTA_PASSWORD "your-ota-password"

// Upload via OTA
pio run --target upload --upload-port <device-ip>
```

## Performance

- **Boot Time**: ~2-3 seconds
- **WiFi Connection**: ~3-5 seconds
- **Audio Capture**: 3-5 seconds (configurable)
- **API Response**: 10-30 seconds (depends on backend)
- **Image Rendering**: 5-15 seconds (e-paper refresh)
- **Total Time**: ~25-60 seconds per sticker

## Memory Usage

- **Flash**: ~1.2 MB (leaves room for OTA)
- **SRAM**: ~120 KB peak
- **PSRAM**: ~2 MB for image buffer (if using WROVER)

## Troubleshooting

### WiFi Won't Connect
- Check SSID and password in config.h
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check router firewall settings

### Audio Quality Issues
- Verify I2S microphone wiring
- Adjust sample rate (16kHz recommended)
- Check noise levels and grounding

### API Errors
- Verify device token is valid
- Check backend API is reachable
- Review serial logs for HTTP status codes

### Display Issues
- Verify SPI wiring to e-paper
- Check power supply (e-paper needs stable 3.3V)
- Adjust refresh timing if ghosting occurs

## Documentation

Detailed documentation available in `/docs`:

- **[QUICKSTART.md](QUICKSTART.md)** - Quick setup guide
- **[HARDWARE.md](HARDWARE.md)** - Detailed wiring diagrams and hardware specs
- **[API.md](API.md)** - Complete API integration guide
- **[SCREEN_ARCHITECTURE.md](docs/SCREEN_ARCHITECTURE.md)** - Display screen system architecture

## Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

## License

MIT License - see LICENSE file for details

## Support

- **Issues**: GitHub Issues
- **Docs**: See `/docs` folder
- **Email**: support@wonderbox.com

## Roadmap

- [x] Basic audio capture and API integration
- [x] SSE streaming support
- [x] E-paper display rendering
- [ ] Multi-language support
- [ ] Offline mode with local caching
- [ ] Battery level monitoring
- [ ] Sleep/wake optimization
- [ ] Web-based configuration portal
- [ ] Bluetooth pairing mode
