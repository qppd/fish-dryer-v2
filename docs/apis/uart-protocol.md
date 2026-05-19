# UART Communication Protocol

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The UART protocol handles communication between the **Arduino Nano** and **NodeMCU Bridge** over SoftwareSerial at 9600 baud. It uses newline-terminated text commands with JSON responses for status.

## 2. Link Configuration

| Parameter | Value |
|-----------|-------|
| Baud Rate | 9600 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Encoding | UTF-8 (ASCII-safe) |
| Termination | `\n` (newline) |
| Line Buffer | 120-128 bytes per side |

## 3. Command Protocol (Any → Nano)

### 3.1 Status Request

```
Command:  STATUS?

Response: {"temp":58.2,"hum":45,"weight":2.50,...}

```

### 3.2 Temperature Setpoint

```
Command:  SETPOINT:65.0

Response: Temperature setpoint set to: 65.0

```

### 3.3 Water Loss Target

```
Command:  WATER_LOSS:70.0

Response: Water loss target set to: 70.0 %

```

### 3.4 Start / Stop / Pause / Resume

```
PID_START
   → "PID thermostat control ENABLED..."
PID_STOP
    → "PID thermostat control DISABLED."
PAUSE
       → "Drying PAUSED."
RESUME
      → "Drying RESUMED."
```

### 3.5 Direct Relay Control

```
SSR1:1
   → "SSR1_PIN set HIGH"
SSR1:0
   → "SSR1_PIN set LOW"
SSR2:1
   → "SSR2_PIN set HIGH"
SSR2:0
   → "SSR2_PIN set LOW"
SSR3:1
   → "SSR3_PIN set HIGH"
SSR3:0
   → "SSR3_PIN set LOW"
```

### 3.6 Load Cell Calibration

```
TARE
                    → "Load cell tared."
CALIBRATE:1.0
          → "Calibration complete. Factor saved to EEPROM."
LOADCELL:RESET
         → "EEPROM cleared. Calibration factor reset to 1.0"
```

### 3.7 Sensor Read Commands (Debug)

```
SHT31:READ
        → "Temp: 58.2 C / Hum: 45 %"
LOADCELL:READ
     → "Weight: 2.500 kg"
PID:READ
          → "PID Status: ENABLED | Current Temp: 58.2 | ..."
PID:SET:65.0
      → "Temperature setpoint set to: 65.0"
```

## 4. Status JSON Format

The full status JSON is sent in response to `STATUS?`:

```json
{
    "temp":        58.2,
    "hum":         45,
    "weight":      2.50,
    "loss":        15.2,
    "heater":      1,
    "fan":         1,
    "exhaust":     0,
    "pid":         1,
    "state":       "DRYING",
    "target_temp": 60.0,
    "target_loss": 70.0,
    "pid_out":     3245,
    "runtime":     3600,
    "fan_a":       0,
    "heater_w":    0,
    "bat_v":       0,
    "sht31":       1
}
```

| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `temp` | float (1 dp) | °C | Chamber temperature |
| `hum` | float (0 dp) | % | Relative humidity |
| `weight` | float (2 dp) | kg | Product weight |
| `loss` | float (1 dp) | % | Water loss |
| `heater` | 0/1 | — | SSR1 relay state |
| `fan` | 0/1 | — | SSR2 relay state |
| `exhaust` | 0/1 | — | SSR3 relay state |
| `pid` | 0/1 | — | PID enabled flag |
| `state` | string | — | IDLE/DRYING/COMPLETE/PAUSED |
| `target_temp` | float (1 dp) | °C | Temperature setpoint |
| `target_loss` | float (1 dp) | % | Water loss target |
| `pid_out` | float (0 dp) | — | PID output value |
| `runtime` | int | seconds | Drying elapsed time |
| `fan_a` | int | A | Fan current (stub) |
| `heater_w` | int | W | Heater power (stub) |
| `bat_v` | int | V | Battery voltage (stub) |
| `sht31` | 0/1 | — | SHT31 health indicator |

## 5. NodeMCU Bridge Polling Sequence

```
NodeMCU                          Nano
   │                               │
   │ ── "STATUS?\n" ──────────────►│
   │                               │
   │ ◄── "{...json...}\n" ─────────│
   │                               │
   │ (parse JSON → EspNowStatusPacket)
   │                               │
   │ (every 2000ms repeat)         │
```

## 6. HMI → Nano Command Chain

```
HMI Touch                     NodeMCU                       Nano
   │                               │                          │
   │ EspNowCmdPacket               │                          │
   │ (0x02, CMD_START_DRYING)      │                          │
   │ ─────────────────────────────►│                          │
   │                               │ "PID_START\n"           │
   │                               │ ────────────────────────►│
   │                               │                          │ handleUARTLine()
   │                               │                          │ → Start drying
   │                               │                          │ → sendStatus()
   │                               │ ◄── "{...json...}\n" ────│
   │ ◄── EspNowStatusPacket ───────│                          │
```

## 7. Error Handling

- **Unknown command:** Nano responds with available commands list
- **Buffer overflow:** Lines > 120 chars silently truncated
- **No reply:** NodeMCU times out after 500ms
- **Corrupted JSON:** Filtered (only lines starting with `{` are accepted)
