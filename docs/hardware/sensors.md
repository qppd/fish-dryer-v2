# Sensor Integration — SHT31 & HX711

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. SHT31 Temperature & Humidity Sensor

### 1.1 Specifications

| Parameter | Value |
|-----------|-------|
| Model | Sensirion SHT31-D |
| Interface | I2C (address 0x44, alt. 0x45) |
| Temperature Range | -40°C to +125°C |
| Temperature Accuracy | ±0.3°C (typical) |
| Humidity Range | 0-100% RH |
| Humidity Accuracy | ±2% RH (typical) |
| Repeatability | High (configurable) |
| Supply Voltage | 2.4-5.5V |

### 1.2 Wiring

| Platform | SDA | SCL | Library |
|----------|-----|-----|---------|
| Arduino Nano | D7 | D8 | SoftwareWire |
| ESP32 | GPIO 21 | GPIO 22 | Wire (Hardware I2C) |

### 1.3 Nano Implementation (Non-blocking State Machine)

The Nano uses **SoftwareWire** on D7/D8 (separate from hardware I2C) to read the SHT31. A non-blocking state machine handles the measurement cycle:

```
TIMELINE (2-second cycle):
  
  T=0ms:     Send command 0x2400 (single-shot, high repeatability)
             └─ If ACK → transition to WAITING
             └─ If NACK → fail counter++
  
  T=20ms:    Request 6 bytes from sensor
             └─ Parse: temp_raw (bytes 0-1), temp_crc (byte 2),
                        hum_raw (bytes 3-4), hum_crc (byte 5)
  
  T=20ms:    Validate CRC-8 for both temperature and humidity
  
  T=20ms:    Convert raw values:
             temp = -45°C + 175°C × (rawT / 65535)
             hum  = 100% × (rawH / 65535)
  
  T=20ms+:   Range check (-40..125°C, 0..100%RH)
  
  T=2000ms:  Repeat from T=0
```

### 1.4 CRC-8 Implementation

**Polynomial:** 0x31 (x⁸ + x⁵ + x⁴ + 1)  
**Initial value:** 0xFF  
**Final XOR:** None

```cpp
static uint8_t _sht31CRC(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}
```

**Validation:** Temperature CRC at byte 2 validates bytes 0-1. Humidity CRC at byte 5 validates bytes 3-4.

### 1.5 ESP32 Implementation

Uses `Adafruit_SHT31` library which handles the I2C protocol internally. Returns `NaN` on read failure.

```cpp
void updateSensors() {
    float temp = sht31.readTemperature();
    float hum = sht31.readHumidity();
    if (!isnan(temp)) CURRENT_TEMPERATURE = temp;
    if (!isnan(hum)) CURRENT_HUMIDITY = hum;
}
```

## 2. HX711 Load Cell

### 2.1 Specifications

| Parameter | Value |
|-----------|-------|
| ADC Resolution | 24-bit (delta-sigma) |
| Interface | 2-wire serial (DOUT, SCK) |
| Channels | 2 differential / 1 single-ended |
| Gain | 128 (ch A) / 64 (ch B) |
| Data Rate | 10 or 80 SPS |
| Supply | 2.7-5.5V |

### 2.2 Wiring

| Platform | DOUT | SCK |
|----------|------|-----|
| Arduino Nano | D2 | D3 |
| ESP32 | GPIO 34 | GPIO 27 |

### 2.3 Calibration Factor

**Default:** `-10697.956054`

The negative sign indicates inverted wiring polarity. This factor converts raw ADC counts to kilograms:

```
Weight (kg) = ADC_raw_reading / CALIBRATION_FACTOR
```

### 2.4 Nano: EEPROM Calibration Persistence

The Nano stores the calibration factor in its internal EEPROM:

| Address | Size | Content |
|---------|------|---------|
| 0 | 1 byte | Magic byte (0xBF = valid) |
| 1-4 | 4 bytes | Calibration factor (IEEE 754 float) |

**Boot sequence:**
1. Wait for HX711 ready (max 3 seconds)
2. Check EEPROM magic byte at address 0
3. If 0xBF, read factor from bytes 1-4 and validate:
   - Must be finite (`isfinite()`)
   - Must be > 0
   - Must be < 999999
4. If valid → `scale.set_scale(factor)`
5. If invalid → clear magic, use default factor
6. Always: `scale.tare()` after initialization

**Calibration procedure:**
1. `TARE` — Zero the scale with empty platform
2. Place known weight (e.g., 1.000 kg)
3. `CALIBRATE:1.0` — Calculate and save factor

**Reset:** `LOADCELL:RESET` → Clears EEPROM magic, factor = 1.0, re-tares.

### 2.5 ESP32: Fixed Factor

The ESP32 firmware uses the same default factor but **does not** implement EEPROM persistence. Calibration must be re-entered on each boot.

### 2.6 Stability Measures

| Technique | Nano | ESP32 |
|-----------|------|-------|
| Averaging | 10 samples | 5 samples |
| Negative clamp | 0 | 0 (via `if < 0 = 0`) |
| Readiness check | `scale.is_ready()` | `scale.is_ready()` |
| Power-up delay | 500ms + wait loop (3s max) | — |

### 2.7 Known Failure Modes

| Symptom | Likely Cause | Solution |
|---------|-------------|----------|
| Always reads 0 | Not calibrated | Send TARE then CALIBRATE command |
| Erratic flips | EMI noise | Shield cable, move away from SSRs |
| Stalls at old value | HX711 not ready | Check DOUT/SCK wiring |
| Negative readings | No tare done | Send TARE command |
