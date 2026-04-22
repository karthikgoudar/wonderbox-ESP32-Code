# Hardware Setup Guide

## Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| ESP32 Development Board | ESP32-DevKitC or ESP32-WROVER | 1 | WROVER recommended for PSRAM |
| I2S Microphone | INMP441 | 1 | Digital MEMS microphone |
| E-Paper Display | Waveshare 7.5" 800x480 | 1 | Black/White, SPI interface |
| Push Button | Momentary tactile switch | 1 | Normally open |
| LED | 5mm LED (any color) | 1 | Status indicator |
| Resistor | 220Ω | 1 | For LED current limiting |
| Resistor | 10kΩ | 1 | Button pull-up (optional) |
| Breadboard | Full-size | 1 | For prototyping |
| Jumper Wires | M-M, M-F | ~20 | Various lengths |
| Power Supply | 5V 2A USB | 1 | For ESP32 |

## Wiring Diagrams

### ESP32 Pinout Reference

```
                        ESP32-DevKitC
                    ┌─────────────────┐
                    │                 │
         3.3V ──────┤ 3V3         VIN ├────── 5V
         GND  ──────┤ GND         GND ├────── GND
         LED  ──────┤ IO2         IO4 ├
                    │ IO0         IO5 ├
    Button   ──────┤ IO0(BOOT)  IO18 ├────── EPD SCK
                    │ IO5        IO19 ├
                    │ IO18       IO21 ├
                    │ IO19       IO23 ├────── EPD MOSI
                    │ IO21       IO22 ├
                    │ IO23       IO25 ├────── I2S WS / EPD BUSY
                    │ IO22       IO26 ├────── I2S SCK / EPD RST
         I2S SD ────┤ IO27       IO27 ├────── EPD DC
                    │ IO26       IO15 ├────── EPD CS
                    │            IO13 ├
                    │            IO12 ├
                    │            IO14 ├
                    │            GND  ├────── GND
                    │            3V3  ├────── 3.3V
                    └─────────────────┘
```

### I2S Microphone (INMP441) Wiring

```
    INMP441 Module          ESP32
    ┌────────────┐          ┌──────┐
    │            │          │      │
    │ VDD        ├──────────┤ 3.3V │
    │ GND        ├──────────┤ GND  │
    │ SD         ├──────────┤ IO27 │
    │ WS (LRCL)  ├──────────┤ IO25 │
    │ SCK (BCLK) ├──────────┤ IO26 │
    │ L/R        ├──────────┤ GND  │ (Left channel)
    └────────────┘          └──────┘
```

**Important Notes:**
- L/R pin: Connect to GND for left channel, 3.3V for right channel
- Keep wires short to minimize noise (<10cm recommended)
- Use shielded cable if possible for audio quality
- VDD must be 3.3V (NOT 5V - will damage microphone)

### E-Paper Display (Waveshare 7.5") Wiring

```
    E-Paper Display        ESP32
    ┌────────────┐         ┌──────┐
    │            │         │      │
    │ VCC        ├─────────┤ 3.3V │
    │ GND        ├─────────┤ GND  │
    │ DIN (MOSI) ├─────────┤ IO23 │
    │ CLK (SCK)  ├─────────┤ IO18 │
    │ CS         ├─────────┤ IO15 │
    │ DC         ├─────────┤ IO27 │
    │ RST        ├─────────┤ IO26 │
    │ BUSY       ├─────────┤ IO25 │
    └────────────┘         └──────┘
```

**Important Notes:**
- Display uses SPI interface
- BUSY pin indicates when display is ready
- Do NOT use 5V - e-paper displays are 3.3V only
- Some displays have different pin labels (DIN=MOSI, CLK=SCK)

### Button Wiring

```
                       ┌──────┐
                       │ ESP32│
         ┌─────────────┤ IO0  │
         │             │      │
         │      10kΩ   │ 3.3V │
         │     ┌───────┤      │
    ┌────┴────┐│       └──────┘
    │  Button ││
    │   (NO)  │└───────┬
    └────┬────┘        │
         │             │
         └─────────────┴─── GND
```

**Notes:**
- Button connects IO0 to GND when pressed
- Internal pull-up can be used (remove 10kΩ resistor)
- IO0 is BOOT button on most DevKits - you can use this directly

### LED Wiring

```
         ┌──────┐
         │ ESP32│
         │      │
         │ IO2  ├────┬
         │      │    │
         │ GND  ├─┐  │ 220Ω
         └──────┘ │  ├─────┐
                  │  │     │
                  │  │    ─┴─ LED (Anode)
                  │  │    ─┬─     (Cathode)
                  │  │     │
                  └──┴─────┘
```

**Notes:**
- 220Ω resistor limits current to ~10mA @ 3.3V
- LED orientation: Long leg (anode) to resistor, short leg (cathode) to GND
- GPIO2 is often connected to built-in LED on DevKits

## Complete Breadboard Layout

```
                     Breadboard Layout
    ┌─────────────────────────────────────────────┐
    │                                             │
    │    ┌────────────┐                          │
    │    │   INMP441  │      ┌──────────┐        │
    │    │  MIC       │      │  ESP32   │        │
    │    └─┬─┬─┬─┬─┬──┘      │ DevKitC  │        │
    │      │ │ │ │ │         └─┬─┬─┬─┬──┘        │
    │  ────┴─┴─┴─┴─┴───────────┴─┴─┴─┴────       │
    │   VDD GND SD WS SCK     3V3 GND 27 25 26   │
    │                                             │
    │         ┌──────────────────┐               │
    │         │  E-Paper Display │               │
    │         │  7.5" Waveshare  │               │
    │         └─┬─┬─┬─┬─┬─┬─┬─┬──┘               │
    │       ────┴─┴─┴─┴─┴─┴─┴─┴────              │
    │      VCC GND DIN CLK CS DC RST BUSY        │
    │      3.3 GND  23  18  15 27  26  25        │
    │                                             │
    │    Button     LED                          │
    │    ┌───┐    ┌─┴─┐                         │
    │  ──┤   ├──  ├ R ├──                       │
    │    └─┬─┘    └─┬─┘                         │
    │      IO0      IO2                          │
    │                                             │
    └─────────────────────────────────────────────┘
```

## Pin Configuration Summary

| Function | ESP32 Pin | Connected To | Notes |
|----------|-----------|--------------|-------|
| I2S WS | GPIO 25 | INMP441 WS | Word Select |
| I2S SCK | GPIO 26 | INMP441 SCK | Bit Clock |
| I2S SD | GPIO 27 | INMP441 SD | Serial Data |
| EPD CS | GPIO 15 | Display CS | Chip Select |
| EPD DC | GPIO 27 | Display DC | Data/Command |
| EPD RST | GPIO 26 | Display RST | Reset |
| EPD BUSY | GPIO 25 | Display BUSY | Busy Signal |
| SPI MOSI | GPIO 23 | Display DIN | SPI Data |
| SPI SCK | GPIO 18 | Display CLK | SPI Clock |
| Button | GPIO 0 | Button | Boot Button |
| LED | GPIO 2 | LED | Built-in LED |

**Pin Conflict Warning:**
- GPIO 25, 26, 27 are used by BOTH microphone and display
- This example shows conflicts - you MUST reassign pins in config.h
- Recommended fix:
  - Move EPD BUSY to GPIO 33
  - Move EPD RST to GPIO 32
  - Move EPD DC to GPIO 14

## Power Considerations

### Power Supply
- **Input**: 5V via USB or external supply
- **Regulation**: ESP32 has built-in 3.3V regulator
- **Current Draw**:
  - ESP32 idle: ~80mA
  - ESP32 WiFi active: ~200mA
  - INMP441: ~1.5mA
  - E-Paper refresh: ~30mA peak
  - **Total**: ~250mA typical, 300mA peak

### Battery Operation (Optional)
- **Recommended**: 3.7V LiPo battery (2000mAh+)
- **Required**: Battery charging module (TP4056)
- **Expected Runtime**: 6-8 hours with deep sleep
- **Wake-up**: Button press triggers ESP32 wake

### Deep Sleep Current
- ESP32 in deep sleep: ~10μA
- Total with peripherals off: ~50μA
- Battery life (2000mAh): ~1600 hours (~66 days)

## Assembly Tips

1. **Start Small**: Test each component individually first
2. **Microphone First**: Verify I2S audio capture works
3. **Display Second**: Test display rendering separately
4. **Integration**: Combine components only after individual testing
5. **Power Measurement**: Use multimeter to check current draw
6. **Serial Monitor**: Always monitor serial output during testing

## Testing Checklist

- [ ] ESP32 powers on, serial output visible
- [ ] WiFi connects successfully
- [ ] Button press detected (LED blinks)
- [ ] Microphone captures audio (check serial for I2S reads)
- [ ] Display shows welcome screen
- [ ] HTTP POST to API succeeds
- [ ] SSE stream receives events
- [ ] Image downloads and renders
- [ ] Complete end-to-end flow works
- [ ] Error recovery functions correctly
- [ ] Deep sleep and wake cycle works

## Common Issues

### Microphone Not Working
1. Check L/R pin connection (must be GND or 3.3V)
2. Verify I2S pin assignments
3. Check 3.3V power supply
4. Monitor serial for I2S errors
5. Try sample code to test I2S directly

### Display Shows Nothing
1. Check SPI connections (MOSI, SCK, CS)
2. Verify DC, RST, BUSY pins
3. Check 3.3V power
4. Test with manufacturer's sample code
5. Some displays need initialization delay

### GPIO Conflicts
1. Review pinout - avoid pins used for flash/PSRAM
2. Don't use GPIO 6-11 (flash interface)
3. GPIO 0, 2, 15 affect boot mode - be careful
4. Reassign conflicting pins in config.h

## Safety Warnings

⚠️ **DO NOT:**
- Connect 5V to 3.3V pins (will damage components)
- Short circuit power pins
- Exceed current rating of USB power supply
- Connect/disconnect while powered on (can corrupt flash)

✅ **DO:**
- Double-check all connections before powering on
- Use proper current-limiting resistors for LEDs
- Disconnect power before changing wiring
- Monitor serial output during testing

## Next Steps

Once hardware is working:
1. Calibrate microphone gain
2. Tune audio quality settings
3. Optimize power consumption
4. Design custom PCB (optional)
5. Create child-safe enclosure

Happy building! 🔧
