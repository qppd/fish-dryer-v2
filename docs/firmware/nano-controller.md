# Arduino Nano Controller Firmware

**File:** `src/nano/FishDryer/FishDryer.ino`  
**Target:** Arduino Nano (ATmega328P, 16 MHz, 5V)  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The Nano firmware is the **primary production controller** for the Fish Dryer v2. It manages all sensor readings (SHT31 via SoftwareWire, HX711 via dedicated pins), PID temperature control, SSR relay actuation, and bidirectional communication with the NodeMCU bridge over SoftwareSerial.

## 2. Key Differences from ESP32 Firmware

| Feature | Nano | ESP32 |
|---------|------|-------|
| SHT31 Interface | SoftwareWire (D7/D8) | Hardware I2C (Wire) |
| HX711 Calibration | EEPROM-persistent (magic 0xBF) | Fixed default factor |
| Physical Button | D11 (START/STOP toggle) | 4 buttons (stub callbacks) |
| Communication | SoftwareSerial (D9/D10, 9600) | Serial only (115200) |
| EDT Calculation | Not implemented | 10-minute window EDT |
| Humidity Auto-Stop | Not implemented | ≤14% for 5 checks |

## 3. Global State

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `systemState` | `DryingState` | STATE_IDLE | IDLE/DRYING/COMPLETE/PAUSED |
| `CURRENT_TEMPERATURE` | `double` | 0 | °C from SHT31 |
| `TEMPERATURE_SETPOINT` | `double` | 60.0 | Target temperature |
| `CURRENT_HUMIDITY` | `float` | 0 | %RH |
| `CURRENT_WEIGHT` | `float` | 0 | kg from HX711 |
| `INITIAL_WEIGHT` | `float` | 0 | Baseline at drying start |
| `CURRENT_WATER_LOSS` | `float` | 0 | Calculated % |
| `WATER_LOSS_TARGET` | `float` | 70.0 | Auto-stop threshold |
| `PID_ENABLED` | `bool` | false | Master PID control flag |
| `heaterState` | `bool` | false | Relay 1 tracking |
| `fanState` | `bool` | false | Relay 2 tracking |
| `exhaustState` | `bool` | false | Relay 3 tracking |
| `sht31OK` | `bool` | false | SHT31 health flag |

## 4. Core Loop (every 2000ms)

```
1. updateSHT31()        - Non-blocking state machine (triggers measurement, reads later)
2. CURRENT_WEIGHT = readLoadCell()  - 10-sample averaged weight
3. updateWaterLoss()    - Compute water loss %
4. If PID_ENABLED → pidCOMPUTE() → operates SSRs
5. Auto-stop check: if WATER_LOSS >= WATER_LOSS_TARGET → STATE_COMPLETE
```

Status is NOT sent automatically — NodeMCU must send `STATUS?` to request it.

## 5. SHT31 — Non-Blocking State Machine

**File:** `SHT31_CONFIG.h`

Uses `SoftwareWire` on D7/D8 (separate from hardware I2C) with a state machine:

```
SHT_IDLE:
  - Wait 2000ms
  - Send measurement command (0x24, 0x00) — high repeatability
  - If ACK → transition to SHT_WAITING
  - If NACK → increment fail counter

SHT_WAITING:
  - Wait 20ms measurement delay
  - Request 6 bytes from sensor
  - Validate CRC-8 for temperature (bytes 0-1) and humidity (bytes 3-4)
  - Convert: temp = -45 + 175 * (raw/65535)
  - Convert: hum = 100 * (raw/65535)
  - Validate ranges (-40..125°C, 0..100%RH)
  - Return to SHT_IDLE
```

**CRC-8 Implementation (polynomial 0x31):**
```cpp
static uint8_t _sht31CRC(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
    return crc;
}
```

## 6. HX711 — EEPROM Calibration Persistence

**File:** `LOADCELL_CONFIG.h`

| EEPROM Address | Size | Content |
|----------------|------|---------|
| 0 | 1 byte | Magic byte (0xBF indicates valid calibration) |
| 1-4 | 4 bytes | Calibration factor (float, IEEE 754) |

On boot:
1. Read magic byte at address 0
2. If 0xBF → read factor from address 1-4, validate (finite, > 0, < 999999)
3. If valid → `scale.set_scale(factor)`
4. If invalid → reset EEPROM magic to 0, use default factor
5. Always: `scale.tare()` after setting factor

**Calibration command:** `CALIBRATE:<kg>` — uses `scale.calibrate_scale()` internally, saves factor to EEPROM.

**Hard reset:** `LOADCELL:RESET` — clears EEPROM magic, resets factor to 1.0, re-tares.

## 7. Physical Button (D11)

| State | Press Action |
|-------|-------------|
| IDLE or COMPLETE | Start drying (capture weight, enable PID) |
| PAUSED | Resume drying (re-enable PID) |
| DRYING | Stop drying (disable PID, all relays OFF) |

50ms debounce, falling-edge detection (INPUT_PULLUP).

## 8. Command Parser (`handleUARTLine()`)

Single function handles commands from **two sources** simultaneously:
1. `ss` (SoftwareSerial from NodeMCU, 9600 baud)
2. `Serial` (USB from PC, 115200 baud)

Both are processed identically. See [ESP32 Documentation](esp32-controller.md#5-serial-command-interface) for command reference (same format).

## 9. Pin Configuration

| Pin | Function | Notes |
|-----|----------|-------|
| D2 | HX711 DOUT | Load cell data |
| D3 | HX711 SCK | Load cell clock |
| D4 | SSR2 (Fan) | Convection fan |
| D5 | SSR3 (Exhaust) | Exhaust fan |
| D6 | SSR1 (Heater) | Heating element |
| D7 | SHT31 SDA | Software I2C |
| D8 | SHT31 SCL | Software I2C |
| D9 | SS RX | From NodeMCU TX |
| D10 | SS TX | → NodeMCU RX |
| D11 | Button | START/STOP toggle |

## 10. Files

| File | Lines | Purpose |
|------|-------|---------|
| `FishDryer.ino` | 338 | Main firmware |
| `PINS_CONFIG.h` | 87 | Nano-specific pin assignments |
| `PID_CONFIG.h` | 48 | PID wrapper + relay logic |
| `SSR_CONFIG.h` | 15 | SSR definitions |
| `SHT31_CONFIG.h` | 133 | Non-blocking SoftwareWire SHT31 + CRC-8 |
| `LOADCELL_CONFIG.h` | 126 | HX711 with EEPROM calibration |
| `BUTTON_CONFIG.h` | 110 | Button framework (not used in Nano loop) |
| `I2C_COMM.h` | 91 | Legacy I2C slave protocol (reference only) |
