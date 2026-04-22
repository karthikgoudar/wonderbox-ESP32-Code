# WonderBox ESP32 System Flow Documentation

## Overview

**WonderBox** is an IoT device that transforms a child's voice into a custom sticker. The ESP32 captures audio, sends it to a cloud API for processing (speech-to-text + AI image generation), downloads the generated image, displays it on an e-paper screen, and optionally prints it on a thermal printer.

---

## High-Level Architecture

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│   Hardware   │ ───> │   ESP32      │ ───> │  Cloud API   │
│  - Button    │      │  Main Loop   │      │  - Transcribe│
│  - Mic (I2S) │      │  - Capture   │      │  - Generate  │
│  - Display   │      │  - Upload    │      │  - Process   │
│  - Printer   │      │  - Download  │      └──────────────┘
└──────────────┘      └──────────────┘              │
                             ▲                       │
                             └───── SSE Stream ──────┘
```

### Key Components

| Component | Purpose | Files |
|-----------|---------|-------|
| **State Machine** | Orchestrates device states | `state_machine/` |
| **Audio Capture** | Records audio via I2S microphone | `audio/AudioCapture.*` |
| **Processing Audio Player** | Plays looping audio clip during processing | `audio/ProcessingAudioPlayer.*` |
| **HTTP Client** | Handles API requests (upload/download) | `http/HttpClient.*` |
| **SSE Client** | Receives real-time processing updates | `http/SSEClient.*` |
| **Display Manager** | Controls e-paper screen | `display/DisplayManager.*` |
| **Printer Manager** | Sends raster data to thermal printer | `print/PrinterManager.*` |

---

## Complete System Flow

### Phase 1: Initialization (setup)

**Location**: `main.cpp::setup()`

1. **Hardware Setup** (`setupHardware`)
   - Configure button pin with interrupt
   - Configure LED pin
   - Attach button ISR (Interrupt Service Routine)

2. **Display Initialization** (`display.begin()`)
   - Initialize SPI communication with e-paper display
   - Show boot screen

3. **WiFi Connection** (`setupWiFi`)
   - Connect to WiFi network
   - Show connection status on display
   - **Timeout**: 30 seconds (configurable via `WIFI_CONNECT_TIMEOUT_MS`)

4. **Audio System Setup**
   - **AudioCapture**: `audio_capture.begin()`
     - Install I2S driver (microphone input on I2S_PORT)
     - Configure I2S pins (SCK, WS, SD)
     - Allocate audio buffer (WAV header + audio data)
   - **ProcessingAudioPlayer**: `processing_audio_player.begin()`
     - Mount SPIFFS filesystem
     - Install separate I2S driver (speaker output on PROCESSING_AUDIO_I2S_PORT)
     - Configure I2S TX pins

5. **HTTP Client Setup** (`http_client.begin()`)
   - Store API base URL, device token, and child ID
   - Configure request headers

6. **SSE Client Setup** (`sse_client.begin()`)
   - Store API base URL
   - Register event callback function

7. **State Machine** (`state_machine.begin()`)
   - Initialize to `IDLE` state

8. **Printer Manager** (`printer_manager.begin()`)
   - Initialize UART serial for thermal printer
   - Send ESC/POS initialization commands

9. **Show Welcome Screen**
   - Display ready state to user

---

### Phase 2: User Interaction Loop (loop)

**Location**: `main.cpp::loop()`

The main loop continuously:
- Checks for button press events
- Updates SSE client (processes incoming events)
- Updates processing audio player (streams audio chunks)
- Updates display animations
- Updates state machine
- Performs state-specific actions
- Checks for sleep conditions

---

### Phase 3: Recording Flow

**Trigger**: User presses button while in `IDLE` state

**Location**: `main.cpp::performStateMachine()` → `DeviceState::RECORDING`

1. **Button ISR** (`buttonISR`)
   - Sets `button_pressed` flag
   - Triggers state transition: `IDLE` → `RECORDING`

2. **Start Recording** (`audio_capture.startRecording()`)
   - Clear I2S DMA buffer to remove stale data
   - Reserve first 44 bytes for WAV header
   - Read audio data from I2S microphone in chunks
   - Display recording progress bar (0% → 100%)
   - **Duration**: Configurable via `AUDIO_RECORD_DURATION_MS` (default: 3-5 seconds)

3. **Complete Recording**
   - Write WAV header with audio metadata
   - Trigger state transition: `RECORDING` → `UPLOADING`

**Key Files**:
- `audio/AudioCapture.cpp::startRecording()`
- `display/screens/RecordingScreen.h`

---

### Phase 4: Upload Flow

**State**: `DeviceState::UPLOADING`

**Location**: `main.cpp::performStateMachine()` → `DeviceState::UPLOADING`

1. **Prepare Audio Data**
   - Get audio buffer (includes WAV header)
   - Get buffer size

2. **API CALL #1: Submit Audio** (`http_client.submitAudio()`)
   
   **Endpoint**: `POST /api/v1/device/submit`
   
   **Location**: `http/HttpClient.cpp::submitAudio()`
   
   **Request Format**:
   ```
   POST https://api.wonderbox.com/api/v1/device/submit
   Headers:
     x-device-token: <DEVICE_TOKEN>
     Content-Type: multipart/form-data; boundary=----WonderBoxBoundary
   
   Body (multipart):
     - child_id: <CHILD_ID>
     - audio: <audio.wav file data>
   ```
   
   **Response**:
   ```json
   {
     "job_id": "550e8400-e29b-41d4-a716-446655440000"
   }
   ```

3. **Start Processing Audio** (if enabled)
   - `processing_audio_player.startLoop()`
   - Opens `/data/processing.wav` from SPIFFS
   - Begins streaming audio chunks to I2S TX (speaker)
   - Loops continuously until stopped

4. **Connect to SSE Stream** (`sse_client.connect(job_id)`)
   
   **Endpoint**: `GET /api/v1/device/stream/{job_id}`
   
   **Location**: `http/SSEClient.cpp::connect()`
   
   **Request**:
   ```
   GET https://api.wonderbox.com/api/v1/device/stream/{job_id}
   Headers:
     Accept: text/event-stream
     Cache-Control: no-cache
   ```

5. **State Transition**: `UPLOADING` → `PROCESSING`

**Key Files**:
- `http/HttpClient.cpp::submitAudio()`
- `http/SSEClient.cpp::connect()`

---

### Phase 5: Processing Flow (Real-time Updates)

**State**: `DeviceState::PROCESSING`

**Location**: `main.cpp::loop()` → `sse_client.update()` → `handleSSEEvent()`

The SSE connection remains open and streams real-time events from the server:

#### SSE Event Types

1. **status** - Job state changes
   ```
   event: status
   data: {"state": "processing"}
   ```

2. **progress** - Processing step updates
   ```
   event: progress
   data: {"step": "transcribing"}
   ```
   
   **Possible steps**:
   - `"transcribing"` - Converting speech to text
   - `"generating_image"` - Creating sticker with AI

3. **text** - Transcription result
   ```
   event: text
   data: {"text": "I want a dinosaur sticker", "language": "en"}
   ```

4. **image_ready** - Image generation complete
   ```
   event: image_ready
   data: {"image_url": "https://cdn.wonderbox.com/stickers/abc123.png"}
   ```
   
   **Actions**:
   - Stop processing audio player
   - Trigger: `PROCESSING` → `DOWNLOADING`

5. **done** - Job fully complete
   ```
   event: done
   data: {"sticker_id": "abc123"}
   ```

6. **error** - Processing failed
   ```
   event: error
   data: {"message": "Transcription failed: audio too noisy"}
   ```
   
   **Actions**:
   - Stop processing audio
   - Show error screen
   - Return to welcome screen

**Event Processing**: `main.cpp::handleSSEEvent()`

**Key Files**:
- `http/SSEClient.cpp::update()` - Reads SSE stream
- `http/SSEClient.cpp::processLine()` - Parses SSE events
- `main.cpp::handleSSEEvent()` - Handles event callbacks

---

### Phase 6: Download Flow

**State**: `DeviceState::DOWNLOADING`

**Location**: `main.cpp::performStateMachine()` → `DeviceState::DOWNLOADING`

1. **API CALL #2: Download Image** (`http_client.downloadImage()`)
   
   **Endpoint**: `GET <image_url>`
   
   **Location**: `http/HttpClient.cpp::downloadImage()`
   
   **Request**:
   ```
   GET https://cdn.wonderbox.com/stickers/abc123.png
   ```
   
   **Response**: Raw PNG image data
   
   **Implementation**:
   - Open HTTP GET connection to image URL
   - Get content length from response headers
   - Allocate buffer for image data
   - Stream image data in 128-byte chunks
   - Validate complete download

2. **Display Image** (`display.showImage()`)
   - Decode PNG image
   - Render to e-paper display buffer
   - Update display with new image

3. **Print Image** (if enabled via `ENABLE_PRINT_PIPELINE`)
   
   **Location**: `print/PrinterManager.cpp::printImage()`
   
   **Steps**:
   ```
   a) Decode PNG → raw pixel data
   b) Convert to 1-bit monochrome bitmap
   c) Apply dithering for smooth gradients
   d) Convert to ESC/POS raster format
   e) Send raster data to printer via UART
   f) Feed paper and cut
   ```
   
   **Display Updates**:
   - "Preparing print job" (20%)
   - "Printer issue" / "Print complete" (100%)
   - Return to image screen

4. **Free Memory**
   - Free image data buffer

5. **State Transition**: `DOWNLOADING` → `COMPLETE`

**Key Files**:
- `http/HttpClient.cpp::downloadImage()`
- `display/DisplayManager.cpp::showImage()`
- `print/PrinterManager.cpp::printImage()`

---

### Phase 7: Completion

**State**: `DeviceState::COMPLETE`

**Location**: `main.cpp::performStateMachine()` → `DeviceState::COMPLETE`

1. Turn on LED indicator
2. Wait 3 seconds (allow child to view sticker)
3. Reset state machine
4. Return to welcome screen
5. **State Transition**: `COMPLETE` → `IDLE`

---

## API Summary

### API Endpoints Used

| # | Method | Endpoint | Purpose | Request Type | Response Type |
|---|--------|----------|---------|--------------|---------------|
| 1 | POST | `/api/v1/device/submit` | Upload audio for processing | Multipart form data (WAV file) | JSON (job_id) |
| 2 | GET | `/api/v1/device/stream/{job_id}` | Real-time processing updates | SSE connection | Event stream |
| 3 | GET | `<image_url>` | Download generated sticker | - | PNG image binary |

### Authentication

All API requests include device authentication via header:
```
x-device-token: <DEVICE_TOKEN>
```

Configured in `config.h`:
- `DEVICE_TOKEN` - Unique device identifier
- `CHILD_ID` - Associated child profile

---

## State Machine Diagram

```
┌─────────┐
│  IDLE   │ ◄──────────────────────┐
└────┬────┘                        │
     │ Button Press               │
     ▼                             │
┌──────────┐                       │
│RECORDING │                       │
└────┬─────┘                       │
     │ Recording Complete          │
     ▼                             │
┌──────────┐                       │
│UPLOADING │                       │
└────┬─────┘                       │
     │ Audio Uploaded              │
     │ SSE Connected              │
     ▼                             │
┌───────────┐                      │
│PROCESSING │ ◄─── SSE Events ───  │
└────┬──────┘                      │
     │ image_ready event           │
     ▼                             │
┌─────────────┐                    │
│DOWNLOADING  │                    │
└─────┬───────┘                    │
      │ Image Downloaded           │
      │ Image Displayed            │
      │ (Optional: Printed)        │
      ▼                            │
┌──────────┐                       │
│ COMPLETE │ ──────────────────────┘
└──────────┘    (3 second delay)

     ┌──────┐
     │ERROR │ ──► Return to IDLE
     └──────┘
```

---

## Memory Management

### Audio Buffer
- **Allocation**: `audio_capture.begin()`
- **Size**: WAV header (44 bytes) + audio data (~96 KB for 3 seconds)
- **Lifetime**: Allocated once at startup, reused for each recording
- **Deallocation**: `audio_capture.end()` or device restart

### Image Buffer
- **Allocation**: `downloadImage()` - dynamically allocated based on image size
- **Size**: Variable (typically 50-200 KB PNG)
- **Lifetime**: Temporary - freed immediately after display/print
- **Deallocation**: `free(image_data)` after rendering

### Processing Audio
- **Strategy**: **Streaming** (not buffered in memory)
- **File**: `/data/processing.wav` stored in SPIFFS
- **Playback**: Read small chunks (e.g., 256 bytes) on each loop iteration
- **File Handle**: Kept open during processing, closed when stopped

---

## I2S Audio Architecture

The ESP32 uses **two separate I2S peripherals** simultaneously:

### I2S Port 0 (Microphone Input)
- **Mode**: I2S_MODE_RX (Receive)
- **Purpose**: Capture audio from INMP441 microphone
- **Pins**: Configured in `config.h`
  - `I2S_SCK_PIN` - Serial Clock
  - `I2S_WS_PIN` - Word Select
  - `I2S_SD_PIN` - Serial Data Input
- **Sample Rate**: 16 kHz
- **Bits**: 16-bit

### I2S Port 1 (Speaker Output)
- **Mode**: I2S_MODE_TX (Transmit)
- **Purpose**: Play processing audio clip
- **Pins**: Configured in `config.h`
  - `AUDIO_OUT_BCLK_PIN` - Bit Clock
  - `AUDIO_OUT_LRCLK_PIN` - Left/Right Clock
  - `AUDIO_OUT_DOUT_PIN` - Data Output
- **Sample Rate**: 16 kHz
- **Bits**: 16-bit

**Why Two Ports?**
- Recording and playback never happen simultaneously
- However, playback needs to continue during processing state
- Separate drivers prevent conflicts and allow independent configuration

---

## Error Handling

### Error Flow

1. **Error Detected** (any state)
   - Stop processing audio player
   - Log error details
   - Show error screen with message

2. **Error Types**
   - `AUDIO_CAPTURE_FAILED` - Microphone issue
   - `HTTP_REQUEST_FAILED` - Upload failed
   - `HTTP_TIMEOUT` - Request timeout
   - `IMAGE_DOWNLOAD_FAILED` - Download failed
   - `UNKNOWN_ERROR` - SSE error event

3. **Recovery**
   - Display error for 2-5 seconds
   - Reset state machine
   - Return to welcome screen
   - Ready for next attempt

### Timeouts

| State | Timeout | Config Constant |
|-------|---------|-----------------|
| Recording | 10s | `RECORDING_TIMEOUT_MS` |
| Uploading | 30s | `UPLOADING_TIMEOUT_MS` |
| Processing | 120s | `PROCESSING_TIMEOUT_MS` |
| Downloading | 30s | `DOWNLOADING_TIMEOUT_MS` |

---

## Power Management

### Deep Sleep
- **Trigger**: Inactivity timeout (configurable)
- **Wake Source**: Button press (GPIO interrupt)
- **Actions Before Sleep**:
  1. Turn off display
  2. Disconnect WiFi
  3. Configure wake on button press
  4. Enter `esp_deep_sleep_start()`

### Activity Tracking
- `last_activity_time` updated on each button press
- Checked in `StateMachine::update()`

---

## Configuration

All system parameters are defined in `config.h`:

### API Configuration
- `API_BASE_URL` - Cloud API endpoint
- `DEVICE_TOKEN` - Device authentication
- `CHILD_ID` - Associated child profile

### Hardware Pins
- Button, LED, I2S, SPI, UART pins

### Audio Settings
- Sample rate, bit depth, duration
- DMA buffer configuration

### Feature Flags
- `ENABLE_PROCESSING_AUDIO` - Toggle processing audio clip
- `ENABLE_PRINT_PIPELINE` - Toggle thermal printing
- `ENABLE_THERMAL_PRINTER` - Toggle ESC/POS printer support

---

## Key Insights

### Why Driver Install Every Time?
**Fixed**: Added initialization check in `AudioCapture::begin()` to prevent redundant driver installation. The `is_initialized` flag now guards against reinstalling the I2S driver and reallocating buffers.

### Why Keep Audio File Open?
**Design Choice**: The processing audio file is kept open during playback to:
- **Minimize RAM usage** - Streaming small chunks (256 bytes) instead of buffering entire file
- **Enable smooth looping** - File seek is faster than close/reopen
- **Reduce overhead** - Single open/close cycle per processing session

The file is closed immediately when processing completes (image_ready event), which is the correct lifecycle.

### Why Separate I2S Ports?
**Isolation**: Using two I2S peripherals allows microphone and speaker to have independent configurations and prevents driver conflicts.

---

## Debug Tips

### Viewing Logs
- Serial output at 115200 baud
- Logger levels: DEBUG, INFO, WARNING, ERROR
- Each component prefixes logs with name (e.g., "AudioCapture", "HttpClient")

### Testing Without Hardware
- Disable features in `config.h`:
  - `ENABLE_PROCESSING_AUDIO = false` - Skip audio playback
  - `ENABLE_PRINT_PIPELINE = false` - Skip printing
  - `ENABLE_THERMAL_PRINTER = false` - Skip printer UART

### Memory Monitoring
- `Logger::printMemoryStats()` - Shows free heap
- `HttpClient::checkMemory()` - Validates before allocations

---

## Next Steps

For more details, see:
- [API.md](../API.md) - Full API specification
- [HARDWARE.md](../HARDWARE.md) - Hardware setup and pin assignments
- [QUICKSTART.md](../QUICKSTART.md) - Setup and deployment guide
- [SCREEN_ARCHITECTURE.md](SCREEN_ARCHITECTURE.md) - Display system details
