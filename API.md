# API Documentation

## Overview

The WonderBox backend exposes two main endpoints for sticker generation:

1. **Submit Audio** - POST request with audio file
2. **Stream Progress** - Server-Sent Events (SSE) for real-time updates

All requests require device authentication via `x-device-token` header.

## Base URL

```
https://your-backend-domain.com
```

Set in `config.h` as `API_BASE_URL`.

## Authentication

### Device Token

All API requests must include the device token in headers:

```
x-device-token: your-device-token-here
```

The token is a SHA256 hash generated during device registration in the backend.

**Important**: Keep device token secret. It uniquely identifies your device.

## Endpoints

### 1. Submit Audio for Processing

**Endpoint**: `POST /sticker/submit`

**Purpose**: Upload child's audio recording to generate a sticker

**Headers**:
```
Content-Type: multipart/form-data; boundary=----WonderBoxBoundary...
x-device-token: <device-token>
User-Agent: WonderBox-ESP32/1.0
```

**Request Body** (multipart/form-data):
```
------WonderBoxBoundary...
Content-Disposition: form-data; name="child_id"

<child-uuid>
------WonderBoxBoundary...
Content-Disposition: form-data; name="audio"; filename="audio.wav"
Content-Type: audio/wav

<binary WAV audio data>
------WonderBoxBoundary...--
```

**Response** (200 OK):
```json
{
  "job_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

**Error Responses**:

**401 Unauthorized**:
```json
{
  "detail": "Invalid device token"
}
```

**404 Not Found**:
```json
{
  "detail": "Child not found"
}
```

**429 Too Many Requests**:
```json
{
  "detail": "Rate limit exceeded. Try again in 300 seconds"
}
```

**500 Internal Server Error**:
```json
{
  "detail": "Internal server error"
}
```

### 2. Stream Processing Progress

**Endpoint**: `GET /sticker/{job_id}/stream`

**Purpose**: Receive real-time updates about sticker generation progress

**Headers**:
```
Accept: text/event-stream
Cache-Control: no-cache
```

**Response Format**: Server-Sent Events (SSE)

Events are sent as:
```
event: <event-type>
data: <json-data>

```

**Event Types**:

#### Status Event
Sent when job state changes.

```
event: status
data: {"state": "processing"}
```

States: `queued`, `processing`, `done`, `error`, `cancelled`

#### Progress Event
Sent for each processing step.

```
event: progress
data: {"step": "transcribing"}
```

Steps:
- `starting` - Job initialized
- `validating_device` - Checking device authorization
- `validating_child` - Checking child exists
- `checking_limits` - Verifying rate limits
- `transcribing` - Converting audio to text (STT)
- `translating` - Translating to English (if needed)
- `normalizing_prompt` - Building age-appropriate prompt
- `generating_image` - Creating sticker with AI
- `processing_image` - Converting to coloring book style
- `uploading_to_storage` - Saving to cloud storage
- `saving_to_db` - Persisting to database

#### Text Event
Sent after successful transcription.

```
event: text
data: {"text": "a cute dragon", "language": "en"}
```

#### Image Ready Event
Sent when sticker image is available for download.

```
event: image_ready
data: {"image_url": "https://storage.example.com/stickers/abc123.png"}
```

#### Done Event
Sent when entire process completes successfully.

```
event: done
data: {"sticker_id": "660e8400-e29b-41d4-a716-446655440000"}
```

#### Error Event
Sent if any step fails.

```
event: error
data: {"message": "Image generation failed", "code": "GENERATION_ERROR"}
```

**Connection Behavior**:
- Stream stays open until terminal event (`done`, `error`, or `cancelled`)
- Client should close connection after terminal event
- Server may close connection after timeout (~2 minutes)
- Client should handle reconnection if needed

## Complete Flow Example

### Step 1: Submit Audio

```http
POST /sticker/submit HTTP/1.1
Host: api.wonderbox.com
x-device-token: abc123def456...
Content-Type: multipart/form-data; boundary=----WonderBoxBoundary

------WonderBoxBoundary
Content-Disposition: form-data; name="child_id"

550e8400-e29b-41d4-a716-446655440000
------WonderBoxBoundary
Content-Disposition: form-data; name="audio"; filename="audio.wav"
Content-Type: audio/wav

RIFF....WAVEfmt ....data........
------WonderBoxBoundary--
```

**Response:**
```json
{
  "job_id": "770e8400-e29b-41d4-a716-446655440111"
}
```

### Step 2: Connect to SSE Stream

```http
GET /sticker/770e8400-e29b-41d4-a716-446655440111/stream HTTP/1.1
Host: api.wonderbox.com
Accept: text/event-stream
Cache-Control: no-cache
```

**Response Stream:**
```
event: status
data: {"state":"processing"}

event: progress
data: {"step":"transcribing"}

event: progress
data: {"step":"generating_image"}

event: text
data: {"text":"a cute dragon","language":"en"}

event: image_ready
data: {"image_url":"https://cdn.wonderbox.com/stickers/dragon123.png"}

event: done
data: {"sticker_id":"880e8400-e29b-41d4-a716-446655440222"}
```

### Step 3: Download Image

```http
GET https://cdn.wonderbox.com/stickers/dragon123.png HTTP/1.1
```

**Response**: Binary PNG image data

## Rate Limiting

### Limits
- **Requests**: 4 per device per 60 seconds
- **Penalty**: 5 minutes (300 seconds) lockout on violation

### Headers (Optional)
Some implementations may include rate limit headers:
```
X-RateLimit-Limit: 4
X-RateLimit-Remaining: 2
X-RateLimit-Reset: 1234567890
```

### Handling Rate Limits
When you receive `429 Too Many Requests`:
1. Parse retry-after from error message
2. Wait specified duration
3. Retry request
4. Display user-friendly message on device

## Error Handling

### HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | Process response |
| 401 | Invalid token | Check device registration |
| 404 | Not found | Verify child_id exists |
| 429 | Rate limited | Wait and retry |
| 500 | Server error | Retry with backoff |
| 503 | Unavailable | Service down, retry later |

### Retry Strategy

**Exponential Backoff**:
```
delay = base_delay * multiplier^attempt
```

Example (base=1000ms, multiplier=2):
- Attempt 1: Wait 1000ms
- Attempt 2: Wait 2000ms
- Attempt 3: Wait 4000ms
- Max: 30000ms (30 seconds)

**Retryable Errors**:
- HTTP 500 (Internal Server Error)
- HTTP 503 (Service Unavailable)
- Connection timeout
- Network errors

**Non-Retryable Errors**:
- HTTP 401 (Unauthorized)
- HTTP 404 (Not Found)
- HTTP 429 (Rate Limited - use specified wait time)
- Invalid audio format

## Audio Format Requirements

### WAV Specification
- **Format**: PCM WAV
- **Sample Rate**: 16000 Hz (16 kHz)
- **Bit Depth**: 16-bit
- **Channels**: Mono (1 channel)
- **Duration**: 3-5 seconds recommended
- **Max Size**: 2 MB

### WAV Header
Standard 44-byte header:
```
Offset  Size  Field
0       4     "RIFF"
4       4     File size - 8
8       4     "WAVE"
12      4     "fmt "
16      4     16 (chunk size)
20      2     1 (PCM format)
22      2     1 (mono)
24      4     16000 (sample rate)
28      4     32000 (byte rate)
32      2     2 (block align)
34      2     16 (bits per sample)
36      4     "data"
40      4     Data size
44      N     Audio data
```

## Timeouts

### Recommended Values
- **Connection**: 10 seconds
- **Upload**: 30 seconds
- **SSE Stream**: 120 seconds (2 minutes)
- **Image Download**: 30 seconds

### Backend Processing Times (Typical)
- Transcription: 2-5 seconds
- Translation: 1-2 seconds
- Image Generation: 10-30 seconds
- Post-processing: 2-5 seconds
- **Total**: 15-45 seconds

## Security Considerations

1. **HTTPS Only**: Always use encrypted connection
2. **Token Storage**: Store device token securely (not in code)
3. **Validate URLs**: Check image_url domain before downloading
4. **Input Validation**: Verify audio file size before upload
5. **Timeout Protection**: Set maximum wait times
6. **Memory Safety**: Free allocated buffers after use

## Testing with curl

### Submit Audio
```bash
curl -X POST https://api.wonderbox.com/sticker/submit \
  -H "x-device-token: your-token" \
  -F "child_id=550e8400-e29b-41d4-a716-446655440000" \
  -F "audio=@recording.wav"
```

### Stream Progress
```bash
curl -N https://api.wonderbox.com/sticker/<job-id>/stream \
  -H "Accept: text/event-stream"
```

## Monitoring and Debugging

### Log Important Events
- Request start/end times
- HTTP status codes
- Job IDs
- Error messages
- Network failures

### Metrics to Track
- Success rate
- Average processing time
- Error rate by type
- Retry counts
- Network latency

### Serial Output Example
```
[INFO] HttpClient: Submitting audio: 80000 bytes
[DEBUG] HttpClient: Response: {"job_id":"770e8400..."}
[INFO] SSEClient: Connecting to SSE stream
[DEBUG] SSEClient: Event: progress, Data: {"step":"transcribing"}
[INFO] Main: Image ready: https://cdn.wonderbox.com/...
[INFO] HttpClient: Downloaded 125000 bytes
```

## API Versioning

Current version: **v1** (implied in base URL)

Future versions may use:
```
https://api.wonderbox.com/v2/sticker/submit
```

Monitor backend documentation for API changes.

---

For backend implementation details, see `wonderbox-backend` repository.
