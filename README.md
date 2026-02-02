# Fish Dryer v2

Firmware project for an ESP32-based fish dryer controller. The codebase is organized around small configuration headers for each hardware module (relays, sensors, buttons) and a single sketch entry point.

## Status
This repository currently focuses on:

- Hardware bring-up and module-level testing
- Serial-command driven validation (relays, sensor reads, load cell reads)
- Button handling with debounce, short press, and long press detection

## Key Features

- ESP32 (38-pin board) friendly GPIO defaults for outputs
- 3 SSR/relay outputs with simple serial commands
- SHT31 temperature and humidity readings over I2C
- HX711 load cell raw readings
- 4 tactile buttons using `INPUT_PULLUP`
- Non-blocking, `millis()`-based button debounce and press classification

## Repository Layout

```
src/
  esp/
    FishDryer/
      FishDryer.ino
      SSR_CONFIG.h
      SHT31_CONFIG.h
      LOADCELL_CONFIG.h
      BUTTON_CONFIG.h
```

## Hardware Overview

### Relays (SSR Outputs)

Three relay/SSR channels are defined. The naming matches the physical function:

- **SSR1**: Heating Element
- **SSR2**: Convection Fan
- **SSR3**: Exhaust Fan

### Sensors

- **SHT31**: Temperature and humidity sensor (I2C)
- **HX711**: Load cell amplifier (digital interface)

### User Input

- **4 tactile buttons** configured with `INPUT_PULLUP` (active-low)
- Debounced using `millis()`
- Detects short press (press-and-release) and long press (held past threshold)

## Default Pin Assignments (ESP32)

All pins are configurable in the corresponding `*_CONFIG.h` files.

### Relay Pins

| Channel | Function           | Config Symbol | Default GPIO |
|--------:|--------------------|--------------|-------------:|
| SSR1    | Heating Element    | `SSR1_PIN`   | 16           |
| SSR2    | Convection Fan     | `SSR2_PIN`   | 17           |
| SSR3    | Exhaust Fan        | `SSR3_PIN`   | 18           |

### Button Pins

Buttons are active-low because they use `INPUT_PULLUP`.

| Button | Config Index | Default GPIO | Electrical Notes |
|-------:|--------------|-------------:|------------------|
| BTN1   | 0            | 32           | Connect to GND when pressed |
| BTN2   | 1            | 33           | Connect to GND when pressed |
| BTN3   | 2            | 25           | Connect to GND when pressed |
| BTN4   | 3            | 26           | Connect to GND when pressed |

### HX711 Pins

| Signal | Config Symbol         | Default GPIO | Notes |
|--------|------------------------|-------------:|------|
| DOUT   | `LOADCELL_DOUT_PIN`    | 34           | Input-only GPIO is acceptable for DOUT |
| SCK    | `LOADCELL_SCK_PIN`     | 27           | Output GPIO; avoid flash/boot strapping pins |

## Serial Monitor Usage

Use the Arduino/PlatformIO Serial Monitor with:

- Baud rate: `115200`
- Line ending: `Newline` (commands are read using `readStringUntil('\n')`)

## Serial Commands

Commands are case-sensitive.

### Relays

- `SSR1:1` / `SSR1:0` — Heating Element ON/OFF
- `SSR2:1` / `SSR2:0` — Convection Fan ON/OFF
- `SSR3:1` / `SSR3:0` — Exhaust Fan ON/OFF

### Sensors

- `SHT31:READ` — prints temperature (°C) and humidity (%)
- `LOADCELL:READ` — prints an HX711 raw reading

### Buttons

- `BUTTONS:READ` — prints instantaneous button states (PRESSED/RELEASED)

## Button Behavior (Debounce + Press Types)

Button handling is implemented in `BUTTON_CONFIG.h` and is designed to be non-blocking:

- Debounce time: `DEBOUNCE_DELAY` (default 50 ms)
- Long press threshold: `LONG_PRESS_TIME` (default 1000 ms)
- Short press: detected on release if long press did not trigger
- Long press: triggers once per hold

To implement your actual UI behavior, modify these functions:

- `handleShortPress(buttonIndex)`
- `handleLongPress(buttonIndex)`

## Arduino Dependencies

This sketch expects these Arduino libraries to be installed:

- `Adafruit SHT31 Library` (for `Adafruit_SHT31.h`)
- `HX711` (for `HX711.h`)

## Build and Upload

1. Open `src/esp/FishDryer/FishDryer.ino` in the Arduino IDE.
2. Select your ESP32 board and COM port.
3. Install required libraries.
4. Compile and upload.
5. Open Serial Monitor and test using the commands above.

## Safety Notes

- Mains voltage (heating element, fans) is hazardous. Use proper enclosures, fusing, grounding, and isolation.
- SSRs and relay modules must be correctly rated for the load current and voltage.
- Test with low-voltage loads first.

## License

See `LICENSE`.
