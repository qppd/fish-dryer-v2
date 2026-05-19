# System Architecture — Fish Dryer v2

**Document Version:** 1.0.0  
**Author:** Sajed Lopez Mendoza  
**Organization:** QPPD  
**Generated:** Full-system reverse engineering & analysis

---

## 1. System Overview

Fish Dryer v2 is a **multi-microcontroller industrial control system** designed for automated fish drying. It combines three processing nodes, precision sensors, solid-state relay actuators, and a full-color touchscreen HMI into a unified control platform.

### 1.1 Design Principles

- **Modularity** — Each microcontroller has a single, well-defined role
- **Non-Blocking** — All loops use `millis()`-based timing (no `delay()` in critical paths)
- **Safety-First** — Sensor failure detection, automatic shutdown, graceful degradation
- **Off-Grid Ready** — Solar-powered design with LiFePO4 battery bank support

---

## 2. Multi-Node Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     FISH DRYER V2 SYSTEM                         │
│                                                                  │
│  ┌──────────────┐    UART (9600)    ┌──────────────┐             │
│  │  Arduino Nano │ ◄──────────────► │  NodeMCU     │             │
│  │  ATmega328P   │   SS D9(RX)/D10(TX) │  ESP8266    │             │
│  │               │                   │  (Bridge)    │             │
│  │  Controller   │                   │              │             │
│  └───────┬───────┘                   └──────┬───────┘             │
│          │                                  │                     │
│          │ Hardware I/O                     │ ESP-Now (Ch 1)     │
│          │                                  │ Binary Packets     │
│          │                                  │                     │
│          │                   ┌──────────────▼───────┐             │
│          │                   │  ESP32-S3 HMI        │             │
│          │                   │  Waveshare 7" LCD    │             │
│          │                   │  LVGL v8 + Touch     │             │
│          │                   │                      │             │
│          │                   │  - Dashboard         │             │
│          │                   │  - Control Panel     │             │
│          │                   │  - Analytics Charts  │             │
│          │                   │  - Diagnostics       │             │
│          └───────────────────┤                      │             │
│            USB Serial (115200)│  (also USB Serial)  │             │
│                              └──────────────────────┘             │
│                                                                  │
│  ┌──────┐   ┌──────┐   ┌─────┐   ┌────────┐   ┌───────────┐     │
│  │SHT31 │   │HX711 │   │ SSR │   │ SSR    │   │ SSR       │     │
│  │Temp/ │   │Load  │   │ 1   │   │ 2      │   │ 3         │     │
│  │Humid │   │Cell  │   │Heater│   │Convec. │   │Exhaust    │     │
│  └──────┘   └──────┘   └─────┘   └────────┘   └───────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

### 2.1 Node Responsibilities

| Node | MCU | Role | Key Functions |
|------|-----|------|---------------|
| **Nano** | ATmega328P | **Controller** | Read SHT31 (temp/humidity) via SoftwareWire, read HX711 (weight), run PID, control SSRs, auto-stop logic, physical button (D11) |
| **NodeMCU** | ESP8266 | **Bridge** | Relay commands HMI → Nano (ESP-Now → UART), relay status Nano → HMI (UART → ESP-Now), poll Nano every 2s |
| **HMIDisplay** | ESP32-S3 | **HMI** | LVGL v8 UI on 7" TFT, ESP-Now to NodeMCU, touch interaction, charting, alerts, optimistic START/STOP |

### 2.2 Communication Links

| Link | Type | Speed | Protocol | Payload |
|------|------|-------|----------|---------|
| Nano ↔ NodeMCU | UART (SoftwareSerial) | 9600 baud | Plain text / JSON | `{"temp":58.2,...}` JSON or `SETPOINT:60.0` commands |
| NodeMCU ↔ HMI | ESP-Now | ~2 Mbps | Binary packed structs | `EspNowStatusPacket` (38B), `EspNowCmdPacket` (7B) |
| PC ↔ Any Node | USB Serial | 115200 baud | Same text protocol | Debugging / monitoring |

---

## 3. Hardware Architecture

### 3.1 Sensor Layer

```
┌──────────────────────────────────────────────────────────┐
│                    SENSOR LAYER                           │
│                                                          │
│  ┌────────────────────────────────────────────────────┐  │
│  │  SHT31 (Temp/Humidity)                             │  │
│  │  ● I2C Address: 0x44                               │  │
│  │  ● Range: -40..125°C, 0..100% RH                   │  │
│  │  ● Accuracy: ±0.3°C, ±2% RH                        │  │
│  │  ● Protocol: Single-shot, high-repeatability cmd   │  │
│  │  ● CRC-8 (poly 0x31) validated on reads            │  │
│  │  ● Read interval: 2000ms                            │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
│  ┌────────────────────────────────────────────────────┐  │
│  │  HX711 + Load Cell (Weight)                        │  │
│  │  ● 24-bit delta-sigma ADC                          │  │
│  │  ● Calibration factor: -10697.956054 (def)         │  │
│  │  ● EEPROM-persistent calibration                   │  │
│  │  ● 10-sample averaging per reading                  │  │
│  │  ● Negative readings clamped to 0                   │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
│  ┌────────────────────────────────────────────────────┐  │
│  │  Physical Button (D11 - Nano only)                 │  │
│  │  ● INPUT_PULLUP, active LOW                        │  │
│  │  ● 50ms debounce                                    │  │
│  │  ● Toggle behavior: START / STOP / RESUME          │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### 3.2 Actuator Layer

| Relay | Function | Nano Pin | ESP32 Pin | Active | Inactive |
|-------|----------|----------|-----------|--------|----------|
| SSR1 | Heating Element | D6 | GPIO 16 | HIGH | LOW |
| SSR2 | Convection Fan | D4 | GPIO 17 | HIGH | LOW |
| SSR3 | Exhaust Fan | D5 | GPIO 18 | HIGH | LOW |

**Control Pattern (PID mode):**
- `PID_OUTPUT > 0` → SSR1=ON, SSR2=ON, SSR3=OFF (heating + circulation)
- `PID_OUTPUT ≤ 0` → SSR1=OFF, SSR2=OFF, SSR3=ON (exhaust + cooling)

### 3.3 Display Hardware (HMIDisplay)

| Component | Specification |
|-----------|--------------|
| MCU | ESP32-S3 (Xtensa LX7 dual-core) |
| LCD | 7" 800×480 RGB, ST7262 controller |
| Touch | GT911 capacitive touch (I2C) |
| IO Expander | CH422G (I2C address 0x20) |
| Backlight | CH422G GPIO-controlled (switch) |
| Bus | ESP_PANEL_BUS_TYPE_RGB, 16-bit data |
| RGB CLK | 16 MHz |
| LCD Reset | via CH422G IO-3 |
| Touch INT | GPIO 4 |

---

## 4. Software Architecture

### 4.1 Arduino Nano Firmware (`src/nano/FishDryer/`)

```
FishDryer.ino
├── PINS_CONFIG.h     — Nano-specific pin assignments
├── SSR_CONFIG.h      — SSR definitions (references PINS_CONFIG)
├── SHT31_CONFIG.h    — SoftwareWire-based SHT31 non-blocking reader + CRC-8
├── LOADCELL_CONFIG.h — HX711 with EEPROM calibration persistence
├── PID_CONFIG.h      — PID_v1 wrapper (Kp=4.0, Ki=0.0, Kd=22.0, limits 0-5000)
└── BUTTON_CONFIG.h   — 4-button debounce/long-press framework (not used on Nano)
```

**Main loop (every 2000ms):**
1. `updateSHT31()` — non-blocking state machine
2. `readLoadCell()` — 10-sample averaged weight
3. `updateWaterLoss()` — compute water loss %
4. If PID_ENABLED → `pidCOMPUTE()` → SSR control
5. Auto-stop check: water loss ≥ target

**Command handling:**
- Reads from BOTH `Serial` (USB, 115200) and `ss` (SoftwareSerial to NodeMCU, 9600)
- Same command parser for both: `handleUARTLine()`

### 4.2 ESP32 Controller Firmware (`src/esp/FishDryer/`)

```
FishDryer.ino
├── PINS_CONFIG.h     — ESP32-specific pin assignments
├── SSR_CONFIG.h      — SSR definitions
├── SHT31_CONFIG.h    — Adafruit_SHT31 library via Wire (I2C)
├── LOADCELL_CONFIG.h — HX711 with fixed calibration factor
├── PID_CONFIG.h      — PID_v1 wrapper (identical algorithm)
└── BUTTON_CONFIG.h   — 4-button debounce/long-press (used on this platform)
```

Key differences from Nano firmware:
- Uses Hardware I2C (Wire, SDA=21/SCL=22) instead of SoftwareWire
- Uses HX711 default factor (no EEPROM persistence)
- Includes button-based physical interaction (buttons 1-4 on GPIO 32,33,25,26)
- Includes EDT (Estimated Drying Time) calculation and humidity-based auto-stop
- No physical start/stop button (that exists on Nano only)

### 4.3 NodeMCUBridge Firmware (`src/esp/NodeMCUBridge/`)

Acts as protocol translator:
- **ESP-Now side:** Receives `EspNowCmdPacket` from HMI → forwards as text commands to Nano via UART
- **UART side:** Polls Nano every 2s (`STATUS?`), receives JSON → caches as `EspNowStatusPacket` → sends to HMI via ESP-Now
- Sends status to HMI every 2s

### 4.4 HMIDisplay Firmware (`src/esp/HMIDisplay/`)

LVGL v8 HMI with 5 screens:
1. **Boot Screen** — 3s splash with solaraw logo, auto-transitions to Dashboard
2. **Dashboard** — Primary monitoring (temp gauge, humidity/weight cards, water loss bar, relay indicators, elapsed time, EDT, START/STOP)
3. **Control Panel** — Preset selection, temp adjustment, relay toggles, mode selection, water loss slider
4. **Analytics** — Tab-view line charts (temp, humidity, weight), 120-point rolling window
5. **Diagnostics** — Sensor health (dots), power readings, system info, calibration wizard modal

Communication stack:
```
serial_protocol.cpp/h  — ESP-Now transport (calls espnow_protocol.h functions)
dryer_data.h           — Global DryerData struct (shared across screens)
espnow_protocol.h      — Binary packet definitions (shared with NodeMCU)
ui_optimistic_state.*  — Cross-screen optimistic START/STOP state (15s grace)
```

---

## 5. Data Flow Diagrams

### 5.1 Status Data Flow (Nano → HMI)

```
 Nano                     NodeMCU                    HMIDisplay
 ────                     ───────                    ──────────
  │                         │                          │
  │ ◄── "STATUS?" ──────────│                          │
  │                         │  (every 2000ms)          │
  │ ── JSON status ────────►│                          │
  │   {"temp":58.2,...}     │                          │
  │                         │ ── EspNowStatusPacket ──►│
  │                         │   (38 bytes binary)      │
  │                         │   (every 2000ms)         │
  │                         │                          │ applyStatusPacket()
  │                         │                          │   → DryerData
  │                         │                          │   → UI updates (2s timer)
```

### 5.2 Command Data Flow (HMI → Nano)

```
 HMIDisplay                NodeMCU                    Nano
 ──────────                ───────                    ────
  │                          │                          │
  │ User presses START       │                          │
  │                          │                          │
  │ optimistic UI update     │                          │
  │ (immediate)              │                          │
  │                          │                          │
  │ ── EspNowCmdPacket ────►│                          │
  │   (7 bytes)             │                          │
  │                          │ ── "PID_START" ────────►│
  │                          │                          │ handleUARTLine()
  │                          │                          │ → start drying
  │                          │                          │ → sendStatus()
  │                          │                          │
  │ ◄── EspNowStatusPacket ──│                          │
  │   (updated state)        │                          │
```

---

## 6. State Machine

```
                    ┌──────────┐
                    │   IDLE   │
                    └────┬─────┘
                         │ START pressed / PID_START
                         ▼
              ┌──────────────────────┐
         ┌───►       DRYING          │
         │    └──────────┬───────────┘
         │               │
         │    ┌──────────▼───────────┐
         │    │       PAUSED          │
         │    └──────────┬───────────┘
         │               │ RESUME
         │               │
         │    ┌──────────▼───────────┐
         │    │      COMPLETE         │
         │    └──────────────────────┘
         │
         └─── (manual stop from COMPLETE/PAUSED → IDLE)

Transitions:
  IDLE     → DRYING    : START pressed / PID_START command
  DRYING   → COMPLETE  : Water loss ≥ target OR humidity ≤14% for 5 consecutive checks
  DRYING   → PAUSED    : PAUSE command
  PAUSED   → DRYING    : RESUME command
  DRYING   → IDLE      : STOP pressed / PID_STOP command
  PAUSED   → IDLE      : STOP pressed
  COMPLETE → IDLE      : STOP pressed (manual reset)
```

---

## 7. Directory Map

```
src/
├── esp/
│   ├── FishDryer/          -- ESP32 main controller
│   │   ├── FishDryer.ino
│   │   ├── PINS_CONFIG.h
│   │   ├── SSR_CONFIG.h
│   │   ├── SHT31_CONFIG.h
│   │   ├── LOADCELL_CONFIG.h
│   │   ├── PID_CONFIG.h
│   │   └── BUTTON_CONFIG.h
│   ├── HMIDisplay/          -- ESP32-S3 LVGL HMI (21 files)
│   │   ├── HMIDisplay.ino
│   │   ├── dryer_data.h
│   │   ├── serial_protocol.cpp/h
│   │   ├── espnow_protocol.h
│   │   ├── screen_manager.cpp/h
│   │   ├── boot_screen.cpp/h
│   │   ├── dashboard_screen.cpp/h
│   │   ├── control_screen.cpp/h
│   │   ├── analytics_screen.cpp/h
│   │   ├── diagnostics_screen.cpp/h
│   │   ├── manual_operation_screen.cpp/h
│   │   ├── alert_popup.cpp/h
│   │   ├── ui_optimistic_state.cpp/h
│   │   ├── ui_theme.h
│   │   ├── ui_styles.cpp/h
│   │   ├── lvgl_v8_port.cpp/h
│   │   ├── esp_panel_board_custom_conf.h
│   │   └── solaraw.c/h
│   └── NodeMCUBridge/       -- ESP8266 UART↔ESP-Now bridge
│       ├── NodeMCUBridge.ino
│       └── espnow_protocol.h
├── nano/
│   └── FishDryer/           -- Arduino Nano controller
│       ├── FishDryer.ino
│       ├── PINS_CONFIG.h
│       ├── SSR_CONFIG.h
│       ├── SHT31_CONFIG.h
│       ├── LOADCELL_CONFIG.h
│       ├── PID_CONFIG.h
│       ├── BUTTON_CONFIG.h
│       └── I2C_COMM.h       -- Legacy I2C protocol (reference)
└── sample/
    ├── BootSample/          -- Standalone boot screen demo
    ├── NanoUartSample/      -- Nano UART slave test
    └── NODEMCUUartSample/   -- NodeMCU UART master test
```

---

## 8. Technology Stack Summary

| Domain | Technology | Version |
|--------|-----------|---------|
| Controller MCU | ESP32 (Xtensa LX6) / Arduino Nano (ATmega328P) | — |
| HMI MCU | ESP32-S3 (Xtensa LX7) | — |
| Bridge MCU | ESP8266 (NodeMCU 1.0) | — |
| HMI Framework | LVGL | v8.3+ |
| Display Driver | ESP32_Display_Panel (esp_lib_utils) | — |
| PID Library | Arduino-PID-Library (br3ttb) | v1.2.1+ |
| Temp/Humid Sensor | Sensirion SHT31-D | — |
| Load Cell ADC | Avia HX711 (24-bit) | — |
| Communication | ESP-Now (Espressif proprietary) | — |
| UI Fonts | Montserrat (16, 20, 24, 30, 36, 48) | — |
| Display Panel | Waveshare ESP32-S3-Touch-LCD-7 | 800×480 |
| Touch Controller | GT911 (capacitive) | — |
| IO Expander | CH422G | — |

---

**Document Maintainer:** Sajed Lopez Mendoza — QPPD
