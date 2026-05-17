# HMIDisplay Firmware — LVGL Touchscreen HMI

**Directory:** `src/esp/HMIDisplay/`  
**Target:** Waveshare ESP32-S3-Touch-LCD-7 (800x480)  
**Framework:** LVGL v8.3+  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The HMIDisplay provides a full-color touchscreen interface for monitoring and controlling the fish dryer. It communicates with the NodeMCU bridge via ESP-Now binary packets and renders real-time data using LVGL v8.

## 2. File Structure (21 files)

| File | Lines | Purpose |
|------|-------|---------|
| `HMIDisplay.ino` | 100 | Entry point, setup, loop |
| `dryer_data.h` | 102 | Global `DryerData` struct |
| `screen_manager.cpp/h` | 153 | Screen lifecycle + navigation |
| `serial_protocol.cpp/h` | 244 | ESP-Now communication transport |
| `espnow_protocol.h` | 128 | Shared binary packet definitions |
| `boot_screen.cpp/h` | 73 | 3-second splash screen |
| `dashboard_screen.cpp/h` | 529 | Primary monitoring screen |
| `control_screen.cpp/h` | 543 | Presets, relays, mode control |
| `analytics_screen.cpp/h` | 208 | Real-time charting (3 tabs) |
| `diagnostics_screen.cpp/h` | 465 | Sensor health, calibration wizard |
| `manual_operation_screen.cpp/h` | 244 | Step-by-step user guide |
| `alert_popup.cpp/h` | 275 | Alert overlays + drying complete popup |
| `ui_optimistic_state.cpp/h` | 61 | Cross-screen START/STOP optimism |
| `ui_theme.h` | 100 | Colors, fonts, layout constants |
| `ui_styles.cpp/h` | 249 | LVGL style definitions + helpers |
| `lvgl_v8_port.cpp/h` | 1034 | ESP Display Panel LVGL porting |
| `esp_panel_board_custom_conf.h` | 411 | Hardware configuration for Waveshare 7" |
| `solaraw.c/h` | — | Compiled boot logo (C array) |

## 3. `setup()` Flow

```
1. Serial.begin(115200)
2. initDryerData()              → Set defaults
3. Board init                   → ESP Display Panel initialization
   ├── board->init()
   └── board->begin()
4. lvgl_port_init(lcd, touch)  → LVGL on ESP32-S3
5. serialProtoInit()            → ESP-Now setup
6. initStyles()                 → LVGL style objects
7. alertInit()                  → Alert system
8. screenManagerInit()          → Create boot screen
9. lv_timer_create()            → 2s periodic UI update timer
```

## 4. `loop()` 

```cpp
void loop() {
    serialProtoUpdate();  // Handle incoming ESP-Now packets + connection timeout
    delay(10);
}
```

All UI updates are driven by the LVGL timer (2s interval).

## 5. Screen Hierarchy

```
SCREEN_BOOT (3-second splash)
    │ auto-transition (lv_timer, 3000ms, one-shot)
    ▼
SCREEN_DASHBOARD (Home / Primary)
    │
    ├── [Nav buttons] → SCREEN_CONTROL
    ├── [Nav buttons] → SCREEN_ANALYTICS
    └── [Nav buttons] → SCREEN_DIAGNOSTICS
```

Screen switching uses `lv_scr_load_anim(screen, FADE_ON, 300ms)` with `auto_del = true`.

### 5.1 Boot Screen (`boot_screen.cpp`)

- Displays compiled `solaraw` C-array image at ~40% scale
- Subtitle: "Smart Drying System"
- Version label: "v2.0.0"
- Status text: "Initializing..."
- NO animations on logo (static)
- 3-second timer → transitions to Dashboard
- **Critical:** One-shot timer (`repeat_count = 1`); DO NOT call `lv_timer_del()` inside callback

### 5.2 Dashboard (`dashboard_screen.cpp`)

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│ Top Bar: "SolAraw"  [Connection Icon] [State Badge] │
├────────────────────┬────────────────────────────────┤
│                    │  ┌──────┐ ┌──────┐ ┌─────────┐ │
│  Temperature       │  │ Hum  │ │Weight│ │WaterLoss│ │
│  Gauge (lv_meter)  │  │ 64%RH│ │2.5kg │ │  15%    │ │
│  0-120°C           │  └──────┘ └──────┘ └─────────┘ │
│                    │ RELAYS: [●Heater] [●Fan] [○Exh]│
│   58.2°C           │ ████████████░░░░░░░░ (WaterLoss)│
│   Target: 60°C     │ Elapsed: 1h 30m  EDT: 2h 15m  │
├────────────────────┼────────────────────────────────┤
│ [START] [STOP] [SET TEMP]                           │
└─────────────────────────────────────────────────────┘
```

**Widgets:**
- Temperature gauge: 260×260 px, 0-120°C, 3 color arcs (green 0-70, orange 70-90, red 90-120)
- Info cards: Humidity, Weight, Water Loss
- Relay indicators: Green dot (ON) / Gray dot (OFF)
- Water loss progress bar: 420×14 px, 0-100%
- START/STOP buttons with optimistic UI
- EDT display (from Nano's estimatedEDT)

### 5.3 Control Screen (`control_screen.cpp`)

**Layout:**
```
┌──────────────────────────────┬──────────────────────────────┐
│  Drying Preset              │  Drying Mode                 │
│  [Tuyo] [Danggit] [Pusit]   │  ○ Auto  ● Manual            │
│  [Others]                   │                              │
│  ┌──────────────────────┐   │  Target Water Loss           │
│  │ SET: 60 °C           │   │  ├───────────●──────┤ 70%    │
│  │          [+5][+][-][-5] │                              │
│  └──────────────────────┘   │                              │
│  Current: 58.2 °C          │  ┌─────────────────────────┐  │
│                             │  │ ► START DRYING         │  │
│  ──────                     │  ├─────────────────────────┤  │
│  [Heater]   [ON/OFF]       │  │ ■ STOP DRYING          │  │
│  [Fan]      [ON/OFF]       │  └─────────────────────────┘  │
│  [Exhaust]  [ON/OFF]       │                              │
└──────────────────────────────┴──────────────────────────────┘
```

**Presets:**
| Preset | Temperature | Description |
|--------|-------------|-------------|
| Tuyo | 60°C | Salted dried fish |
| Danggit | 60°C | Rabbitfish fillets |
| Pusit | 50°C | Dried squid |
| Others | 30-100°C | Custom (±1°C step or ±5°C) |

**Mode switching:**
- Auto: Relay toggles disabled (PID manages them)
- Manual: All 3 relays are individually toggleable

### 5.4 Analytics Screen (`analytics_screen.cpp`)

Tab view with 3 tabs:
1. **Temperature** (chart range 0-120°C)
2. **Humidity** (chart range 0-100%)
3. **Weight** (chart range auto-adjusted, ~50 to weight*15)

Each chart: LV_CHART_TYPE_LINE, 120 points (4-minute window at 2s intervals), 710×280 px.

### 5.5 Diagnostics Screen (`diagnostics_screen.cpp`)

Two-column layout:
- **Left:** Sensor status (temp, load cell) with green/red dots, scale calibration wizard modal, power readings (fan current, heater power, battery voltage)
- **Right:** System info (connection status, last update, uptime, free heap, HMI version, LVGL version), RUN SENSOR TEST button, calibration wizard button

**Scale Calibration Modal:**
- Semi-transparent overlay (60% black)
- Step 1: TARE scale (empty)
- Step 2: Set known weight (±0.10kg steps)
- Step 3: Place weight → CALIBRATE
- Sends `TARE` / `CALIBRATE:<kg>` commands via ESP-Now

### 5.6 Manual Operation Screen (`manual_operation_screen.cpp`)

Static scrollable guide with 10 steps (Prepare → Load → Tare → Set Temp → Set Water Loss → Start → Monitor → Complete → Unload → Clean). Not connected to navigation buttons in current implementation.

## 6. Optimistic UI System

**Files:** `ui_optimistic_state.cpp/h`

When user presses START/STOP, the UI updates immediately without waiting for the Nano status round-trip (~500ms-2s).

```
Structure:
  UiOptimisticState {
      bool active;             // Is optimistic mode active?
      unsigned long untilMs;   // Expiry timestamp
      DryerState systemState;  // IDLE or DRYING
      bool heaterOn, fanOn, exhaustOn;
      bool showStart, showStop;
  }
```

- **Grace period:** 15,000ms (15 seconds)
- Applied in both Dashboard and Control screens
- On expiry, falls back to live `dryerData.status`
- `uiOptimisticIsActive()` auto-clears if `millis() >= untilMs`

## 7. Alert System (`alert_popup.cpp`)

| Condition | Alert | Type | Frequency |
|-----------|-------|------|-----------|
| Temperature > 95°C | "Temperature too high!" | WARNING | Once |
| Temperature > 110°C | "Critical temperature!" | ERROR | Once |
| Connection lost | "Communication interrupted" | ERROR | Once per 30s |
| STATE_COMPLETE while previously DRYING | Drying Complete overlay | SUCCESS | Once |

Drying Complete overlay shows water loss % and elapsed time.

## 8. Connection Timeout

- `STATUS_TIMEOUT_MS = 6000` (6 seconds)
- If no ESP-Now status packet received within 6s → `dryerData.connected = false`
- Dashboard shows red WiFi icon; Diagnostics shows "Disconnected"

## 9. Communication

All serial communication is handled via `serial_protocol.cpp` which now uses ESP-Now transport (not UART). See [ESP-Now Protocol documentation](../apis/esp-now-protocol.md).

## 10. UI Theme (`ui_theme.h`)

Industrial dark theme:
- Background: `#0F172A` (dark blue)
- Card: `#1E293B` (slate)
- Accent: `#FF6B00` (heat orange)
- Success: `#00D084` (green)
- Danger: `#FF3B30` (red)
- Text: `#FFFFFF` / `#94A3B8`

Font sizes: 16 (small), 20 (medium), 24 (large), 30 (XL), 36 (XXL), 48 (huge) — all Montserrat.

## 11. Display Configuration

**File:** `esp_panel_board_custom_conf.h`
- LCD: ST7262, RGB-565, 800×480, 16-bit data bus
- Touch: GT911, I2C (GPIO 8/9)
- Backlight: CH422G IO expander (switch on/off)
- RGB timing: 16 MHz PCLK, HPW=4, HBP=8, HFP=8, VPW=4, VBP=8, VFP=8
- Avoid tearing mode: 3 (double-buffer + LVGL direct-mode)
