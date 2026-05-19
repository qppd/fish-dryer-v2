# Calculations & Algorithms

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Water Loss Calculation

### Formula

```
Water Loss (%) = ((Initial Weight - Current Weight) / Initial Weight) × 100
```

### Implementation (Nano)

```cpp
void updateWaterLoss() {
    if (INITIAL_WEIGHT > 0.0f) {
        float lost = INITIAL_WEIGHT - CURRENT_WEIGHT;
        if (lost < 0) lost = 0;          // Clamp negative (noise)
        CURRENT_WATER_LOSS = (lost / INITIAL_WEIGHT) * 100.0f;
    } else {
        CURRENT_WATER_LOSS = 0.0f;
    }
}
```

### Implementation (ESP32)

```cpp
if (INITIAL_WEIGHT > 0) {
    WATER_LOSS = ((INITIAL_WEIGHT - CURRENT_WEIGHT) / INITIAL_WEIGHT) * 100.0f;
    if (WATER_LOSS < 0) WATER_LOSS = 0;
}
```

### Example

| Stage | Weight (kg) | Water Loss (%) |
|-------|-------------|----------------|
| Start | 5.000 | 0% |
| Mid | 4.000 | 20% |
| Target | 1.500 | 70% |

---

## 2. PID Temperature Control

### Algorithm

Standard PID_v1 library:

```
Output = Kp × e(t) + Ki × ∫e(t) dt + Kd × de(t)/dt

where:
  e(t) = Setpoint - Current_Temperature  (error)
  Kp   = Proportional gain (4.0)
  Ki   = Integral gain (0.0 — disabled)
  Kd   = Derivative gain (22.0)
```

### Parameters

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Kp | 4.0 | Immediate response to error |
| Ki | 0.0 | Disabled (avoids integral windup) |
| Kd | 22.0 | Strong damping to prevent overshoot |
| Output Min | 0 | Full cooling |
| Output Max | 5000 | Maximum heating |
| Direction | DIRECT | Positive error = increase output |
| Update Interval | 2000 ms | PID recompute rate |

### Relay Logic

```
if (PID_OUTPUT > 0):
    SSR1 = ON  (heating element)
    SSR2 = ON  (convection fan — circulates heated air)
    SSR3 = OFF (exhaust closed — retain heat)
else:
    SSR1 = OFF (heating off)
    SSR2 = OFF (convection off)
    SSR3 = ON  (exhaust open — vent moisture)
```

### Tuning Guide

| Chamber Size | Kp | Ki | Kd |
|-------------|-----|------|-------|
| Small (< 0.5 m³) | 2.0 | 0.5 | 10.0 |
| Medium (0.5-2 m³) | 4.0 | 0.0 | 22.0 |
| Large (> 2 m³) | 6.0 | 0.2 | 30.0 |

---

## 3. Estimated Drying Time (EDT)

**Available in:** ESP32 firmware only (not in Nano)

### Algorithm

```
Window size: 10 minutes (600,000 ms)

Every window:
  deltaWeight = weightAtWindowStart - CURRENT_WEIGHT
  deltaTime   = (now - lastEDTWindowStart) / 1000.0  (seconds)
  dryingRate  = deltaWeight / deltaTime               (kg/s)
  
  targetWeight = INITIAL_WEIGHT × (1 - WATER_LOSS_TARGET / 100)
  weightToLose = CURRENT_WEIGHT - targetWeight
  
  if (dryingRate > 0):
      EDT = weightToLose / dryingRate   (seconds)
  else:
      EDT = -1  (indeterminate — rate not established)
```

### Variables

| Variable | Type | Update | Description |
|----------|------|--------|-------------|
| `lastEDTWindowStart` | unsigned long | Every 10 min | Start of current window |
| `weightAtWindowStart` | float | Every 10 min | Weight at window start |
| `currentDryingRate` | float | Every 10 min | kg/s water loss rate |
| `estimatedEDTSeconds` | long | Every 10 min | Seconds to completion |

### EDT Display on HMI

```
Dashboard EDT label:
  EDT: 2h 15m        (when rate established)
  EDT: ---           (when undetermined / rate = 0)
  (empty)            (when system not in DRYING state)
```

---

## 4. Auto-Stop Trigger Conditions

### Condition 1: Water Loss Target

```
Nano:
  if (systemState == STATE_DRYING && WATER_LOSS_TARGET > 0 &&
      CURRENT_WATER_LOSS >= WATER_LOSS_TARGET):
    → STATE_COMPLETE (all relays OFF)

ESP32:
  (Same logic, using WATER_LOSS_TARGET)
```

Default targets:
- **Nano:** 70% (set via `WATER_LOSS_TARGET` default)
- **ESP32:** 15% default but overridden by HMI slider (10-95%)

### Condition 2: Humidity-Based Stop (ESP32 only)

```
If CURRENT_HUMIDITY <= 14% AND CURRENT_HUMIDITY > 0:
    humidityStopCounter++
    If humidityStopCounter >= 5 (consecutive):
        → stopDrying()
Else:
    humidityStopCounter = 0
```

Threshold: 14% relative humidity  
Required consecutive readings: 5  
At 2s per reading, this requires ~10 seconds below threshold.

---

## 5. EDT Estimation (HMI-Side Alternate)

The Dashboard screen also has a **separate EDT calculation** based on humidity drop rate:

```cpp
#define EDT_TARGET_HUMIDITY_PCT  35.0f
#define EDT_MIN_ELAPSED_MINS     3.0f
#define EDT_MIN_HUMIDITY_DROP    2.0f
```

This is a secondary estimate displayed on the dashboard and may differ from the Nano's weight-based EDT.

---

## 6. HX711 Weight Reading (Averaging)

### Nano Configuration

| Parameter | Value |
|-----------|-------|
| Calibration factor (default) | -10697.956054 |
| Samples per reading | 10 |
| Negative clamp | 0 (no negative weight) |
| EEPROM persistence | Magic 0xBF at byte 0, factor at bytes 1-4 |

### ESP32 Configuration

| Parameter | Value |
|-----------|-------|
| Calibration factor | -10697.956054f (fixed, no EEPROM) |
| Samples per reading | 5 |
| Error value | -1.0 (indicates HX711 not ready) |

---

## 7. Timing Constants

| Constant | Value | Context |
|----------|-------|---------|
| `PID_UPDATE_INTERVAL` | 2000 ms | PID recompute + sensor read cycle |
| `SENSOR_UPDATE_INTERVAL` | 2000 ms | Sensor read cycle (ESP32) |
| `SHT31_READ_INTERVAL_MS` | 2000 ms | SHT31 poll rate (Nano) |
| `SHT31_MEAS_DELAY_MS` | 20 ms | High-repeatability measurement time |
| `EDT_WINDOW_MS` | 600000 ms | EDT rate calculation window |
| `CHART_UPDATE_MS` | 2000 ms | HMI chart data point interval |
| `CHART_POINT_COUNT` | 120 | Chart points = 4 minutes visible |
| `STATUS_TIMEOUT_MS` | 6000 ms | HMI connection timeout |
| `HUMIDITY_STOP_THRESHOLD` | 5 counts | Consecutive ≤14% humidity readings |
| `UIT_OPTIMISTIC_GRACE` | 15000 ms | Optimistic UI grace period |
