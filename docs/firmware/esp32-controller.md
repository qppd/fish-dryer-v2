# ESP32 Controller Firmware

**File:** `src/esp/FishDryer/FishDryer.ino`  
**Target:** ESP32 38-Pin DevKit  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The ESP32 controller firmware manages the entire drying process: reading sensors, running the PID algorithm, controlling SSRs, handling button inputs, and providing a serial command interface for debugging and HMI communication. It uses a **non-blocking** `millis()`-based event loop.

## 2. Global State Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `CURRENT_TEMPERATURE` | `double` | 0 | Latest chamber temperature (°C) |
| `CURRENT_HUMIDITY` | `double` | 0 | Latest chamber humidity (%) |
| `CURRENT_WEIGHT` | `float` | 0 | Latest product weight (kg) |
| `INITIAL_WEIGHT` | `float` | 0 | Weight captured at drying start |
| `WATER_LOSS` | `float` | 0 | Calculated water loss (%) |
| `WATER_LOSS_TARGET` | `float` | 15.0 | Auto-stop water loss threshold (%) |
| `TEMPERATURE_SETPOINT` | `double` | 60.0 | Target temperature (°C) |
| `PID_ENABLED` | `bool` | false | Master PID on/off |
| `PID_OUTPUT` | `double` | 0 | PID computed output (0-5000) |
| `systemState` | `enum DryerState` | IDLE | IDLE/DRYING/COMPLETE/PAUSED |
| `humidityStopCounter` | `int` | 0 | Consecutive low-humidity readings |

## 3. System States

```cpp
enum DryerState { IDLE, DRYING, COMPLETE, PAUSED };
```

See [System Architecture (State Machine)](../architecture/system-architecture.md#6-state-machine) for transition diagram.

## 4. Core Functions

### 4.1 `setup()` — Initialization

1. `Serial.begin(115200)` — Debug console
2. `pinMode(SSR1_PIN..3_PIN, OUTPUT)` — Configure relays
3. `Wire.begin(SDA, SCL)` — I2C bus for SHT31
4. `initSHT31()` — Initialize temperature/humidity sensor
5. `initLoadCell()` — Initialize HX711 + tare
6. `initButtons()` — Configure 4 buttons with INPUT_PULLUP
7. `initPID()` — Configure PID_v1 (AUTOMATIC mode, 0-5000 limits)

### 4.2 `loop()` — Main Execution Cycle

```
Each iteration:
  1. if (2000ms elapsed since last sensor update):
     ├── updateDryingProcess()
     │   ├── updateSensors()           - Read SHT31
     │   ├── getWeightKg()             - Read HX711
     │   ├── Calculate WATER_LOSS     - (initial - current) / initial × 100
     │   ├── Check auto-stop conditions
     │   └── Update EDT (every 10 min window)
     └── lastSensorUpdate = millis()
  
  2. if (PID_ENABLED && 2000ms since last PID update):
     ├── pidCOMPUTE()                 - Compute PID output
     └── lastPIDUpdate = millis()
  
  3. updateButtons()                   - Debounce + short/long press detection
  
  4. Process Serial commands
```

### 4.3 `updateDryingProcess()` — Core Drying Logic

This is called every 2000ms and orchestrates:

1. **Sensor update** — Reads SHT31, updates `CURRENT_TEMPERATURE` and `CURRENT_HUMIDITY`
2. **Weight reading** — `getWeightKg()`, clamped to ≥ 0
3. **Water loss calculation** — Only when `systemState == DRYING` and `INITIAL_WEIGHT > 0`
4. **Auto-stop (humidity)** — If `CURRENT_HUMIDITY ≤ 14%` for 5 consecutive readings → stop
5. **EDT calculation** — Every 10-minute window, compute drying rate

### 4.4 `startDrying()` — Start Sequence

```cpp
INITIAL_WEIGHT = CURRENT_WEIGHT;   // Capture baseline
systemState = DRYING;
PID_ENABLED = true;
dryingStartTime = millis();
lastEDTWindowStart = millis();
weightAtWindowStart = CURRENT_WEIGHT;
humidityStopCounter = 0;
```

### 4.5 `stopDrying()` — Stop Sequence

```cpp
systemState = COMPLETE;  // Not IDLE — user must manually reset
PID_ENABLED = false;
operateSSR(1, OFF);  // Heater off
operateSSR(2, OFF);  // Fan off
operateSSR(3, OFF);  // Exhaust off
```

### 4.6 `sendStatusJSON()` — Status Broadcast

Sends a JSON line to Serial (115200 baud) with all current values:

```json
{"state":"DRYING","temp":58.20,"hum":45.30,"weight":2.500,"loss":15.20,
 "target_temp":60.00,"target_loss":70.00,"pid_out":3245.00,"runtime":3600,
 "heater":1,"fan":1,"exhaust":0,"pid":1,"sht31":1}
```

The NodeMCUBridge reads this line and forwards it to the HMI via ESP-Now.

## 5. Serial Command Interface

All commands are newline-terminated at 115200 baud.

| Command | Action |
|---------|--------|
| `STATUS?` | Returns full JSON status |
| `PID_START` | Start drying cycle |
| `PID_STOP` | Stop drying (state → IDLE) |
| `TARE` | Zero the load cell |
| `SETPOINT:65.0` | Set target temperature (°C) |
| `WATER_LOSS:50.0` | Set target water loss (%) |
| `SHT31:READ` | Read and print SHT31 values |
| `LOADCELL:READ` | Read and print weight |
| `SSR1:1` / `SSR1:0` | Turn heater ON/OFF |
| `SSR2:1` / `SSR2:0` | Turn convection fan ON/OFF |
| `SSR3:1` / `SSR3:0` | Turn exhaust fan ON/OFF |

## 6. Configuration Files

### 6.1 `PINS_CONFIG.h`

See [Hardware Wiring & Pinout](../hardware/wiring-and-pinout.md#3-esp32-pin-assignments-38-pin-devkit).

### 6.2 `PID_CONFIG.h`

```cpp
#define PID_KP 4.0
#define PID_KI 0.0
#define PID_KD 22.0
PID pid(&CURRENT_TEMPERATURE, &PID_OUTPUT, &TEMPERATURE_SETPOINT, KP, KI, KD, DIRECT);
pid.SetOutputLimits(0, 5000);
```

**`pidCOMPUTE()`** logic:
- If PID output > 0: SSR1=ON (heat), SSR2=ON (fan), SSR3=OFF (exhaust)
- If PID output ≤ 0: SSR1=OFF, SSR2=OFF, SSR3=ON (exhaust moisture)

### 6.3 `SHT31_CONFIG.h`

Uses `Adafruit_SHT31` library via hardware I2C (`Wire`). Returns `NaN` on failure.

### 6.4 `LOADCELL_CONFIG.h`

Uses fixed calibration factor `-10697.956054f` (no EEPROM). See [Load Cell documentation](../hardware/sensors.md#hx711-load-cell).

### 6.5 `BUTTON_CONFIG.h`

4-button debounce framework with 50ms debounce and 1000ms long-press. Actions are stubs — user must implement callback functions.

## 7. Files

| File | Lines | Purpose |
|------|-------|---------|
| `FishDryer.ino` | 212 | Main firmware with setup/loop/serial command handling |
| `PINS_CONFIG.h` | 73 | Centralized GPIO pin assignments |
| `PID_CONFIG.h` | 48 | PID controller wrapper + relay logic |
| `SSR_CONFIG.h` | 15 | SSR definitions |
| `SHT31_CONFIG.h` | 46 | SHT31 init + read functions |
| `LOADCELL_CONFIG.h` | 51 | HX711 init + weight reading |
| `BUTTON_CONFIG.h` | 110 | 4-button debounce/long-press framework |
