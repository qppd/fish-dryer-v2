# Troubleshooting Guide

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. SHT31 Issues

### 1.1 "Couldn't find SHT31 sensor!"

| Possible Cause | Check | Solution |
|----------------|-------|----------|
| Wiring | SDA/SCL connections | Verify SDA=D7, SCL=D8 (Nano) or GPIO21/22 (ESP32) |
| Address | Default 0x44 | If sensor has ADDR pin HIGH → use 0x45 |
| Pull-up resistors | I2C bus requires pull-ups | Add 4.7kΩ resistors to VCC on SDA/SCL |
| Power | Sensor needs 2.4-5.5V | Check sensor VCC connection |
| SoftwareWire issue | Nano-specific | Try reducing I2C speed or adding 10ms delay between reads |

### 1.2 CRC-8 Validation Fails Frequently

| Cause | Solution |
|-------|----------|
| Electrical noise | Shield cable, move away from SSR/heater lines |
| Long wires (>20cm) | Keep I2C wires short, use twisted pair |
| Missing pull-ups | Add 4.7kΩ pull-up resistors |
| Interference from SSRs | Route sensor wires away from high-current lines |

### 1.3 Sporadic NaN Readings (ESP32)

The ESP32 uses the Adafruit_SHT31 library which returns `NaN` on communication failure. The firmware handles this with `isnan()` checks.

## 2. Load Cell / HX711 Issues

### 2.1 "HX711 not ready!" or Always Returns 0

| Cause | Check | Solution |
|-------|-------|----------|
| Wiring | DOUT=D2, SCK=D3 (Nano) | Re-check connections |
| Power | HX711 needs 2.7-5.5V | Ensure stable power |
| Initialization timing | HX711 needs stabilization time | Firmware waits 500ms + 3s max |
| Not calibrated | Boot message | Send TARE then CALIBRATE:<kg> |
| DOUT stuck high | Hardware issue | Re-power HX711, check solder joints |

### 2.2 Erratic / Jumpy Readings

| Symptom | Cause | Solution |
|---------|-------|----------|
| ±100g fluctuations | EMI from SSRs | Move HX711 away from power lines |
| ±500g swings | Loose wiring | Check all screw terminals |
| Reading drifts | Temperature change | Allow 5min warm-up before tare |
| Reading flips sign | Wire polarity | Invert calibration factor sign |

**Shielding tips:**
- Use shielded twisted-pair cable for load cell
- Keep HX711 module at least 10cm from SSR heatsinks
- Add ferrite bead on load cell cable near HX711

### 2.3 EEPROM Calibration Lost

| Symptom | Cause | Solution |
|---------|-------|----------|
| "using default factor" on boot | First boot | Send CALIBRATE:<kg> once |
| "EEPROM factor corrupted" | ESD or write error | Re-send CALIBRATE:<kg> |
| Calibration drifts over time | Load cell creep | Re-calibrate monthly |

## 3. PID Control Issues

### 3.1 Temperature Oscillation (Hunting)

```
Symptom: Temp swings ±10°C around setpoint every 2-5 minutes
```

| Fix | Current Value | Try |
|-----|---------------|-----|
| Reduce Kp | 4.0 | 2.0 or 1.0 |
| Increase Kd | 22.0 | 30.0 or 40.0 |
| Add Ki | 0.0 | 0.5 (with anti-windup) |
| Increase update interval | 2000ms | 5000ms |

### 3.2 Slow to Reach Setpoint

```
Symptom: Takes >30 minutes to reach 60°C from ambient
```

| Fix | Current | Try |
|-----|---------|-----|
| Increase Kp | 4.0 | 6.0 or 8.0 |
| Reduce Kd | 22.0 | 10.0 or 5.0 |
| Check heater power | — | Verify SSR1 is actually switching |

### 3.3 Relay Chatter (Rapid ON/OFF)

```
Symptom: SSR1 clicks ON/OFF every 2 seconds
```

| Cause | Solution |
|-------|----------|
| Setpoint too close to ambient | Increase target temperature |
| Kd too low | Increase Kd to 30.0 |
| Heater oversized for chamber | Add hysteresis in output |

## 4. Communication Issues

### 4.1 HMI Shows "Disconnected"

| Cause | Check | Solution |
|-------|-------|----------|
| MAC mismatch | ESP-Now peer addresses | Re-run pairing procedure |
| Channel mismatch | Wi-Fi channel both devices | Ensure same channel (default 1) |
| NodeMCU not powered | — | Check NodeMCU LED |
| Nano not responding | — | Check Nano Serial output |
| ESP-Now range | Distance too great | Keep within 50m line-of-sight |

### 4.2 ESP-Now Send Failures

The NodeMCU prints `[ESP-Now] Send failed — is HMI_PEER_MAC correct?` when `esp_now_send()` returns non-zero status.

| Check | What to verify |
|-------|----------------|
| `HMI_PEER_MAC` in NodeMCU | Matches HMIDisplay's printed MAC |
| `NODEMCU_PEER_MAC` in HMI | Matches NodeMCU's printed MAC |
| Both on same channel | `ESPNOW_WIFI_CHANNEL` same in both |
| Both in STA mode | Both call `WiFi.mode(WIFI_STA)` |

### 4.3 UART Data Corruption

| Symptom | Cause | Solution |
|---------|-------|----------|
| Garbled Nano output | 9600 baud mismatch | Verify both sides use 9600 |
| Missing characters | SoftwareSerial buffer | Reduce line length (<120 chars) |
| No response from Nano | Level shift missing on D10 | Add voltage divider! |

## 5. HMI Display Issues

### 5.1 LVGL Crash / LoadProhibited

**Known pitfall:** Do NOT call `lv_timer_del()` inside the timer's own callback when using `repeat_count = 1`. LVGL handles deletion automatically; manual deletion causes use-after-free.

### 5.2 Display Not Initializing

| Symptom | Likely Cause |
|---------|--------------|
| No backlight | CH422G IO expander issue |
| White screen | LCD initialization failed |
| Partial display | RGB timing mismatch (HPW/HBP/HFP) |

### 5.3 Touch Not Working

| Check | Value |
|-------|-------|
| GT911 I2C address | 0x5D or 0x14 |
| I2C pins | SCL=GPIO9, SDA=GPIO8 |
| Interrupt pin | GPIO4 |
| Reset pin | CH422G IO-1 |

## 6. General Debugging

### 6.1 Test with Serial Monitor

Connect to Nano USB Serial at 115200 baud:

```
> SHT31:READ
Temp: 58.2 C
Hum: 45 %

> LOADCELL:READ
Weight: 2.500 kg

> SSR1:1
SSR1_PIN set HIGH

(Wait 10 seconds)

> SHT31:READ
Temp: 35.2 C, Humidity: 42.1 %
```

### 6.2 Reset Calibration

```
> TARE
> LOADCELL:RESET
> TARE
> CALIBRATE:1.0
```

### 6.3 Node Isolation Test

1. Disconnect NodeMCU from Nano UART
2. Test Nano with USB Serial only
3. Test NodeMCU with UART sample sketch
4. Test HMI with BootSample sketch
