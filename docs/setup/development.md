# Development Environment & Setup

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Prerequisites

### 1.1 Hardware Required

- Arduino Nano (ATmega328P)
- NodeMCU 1.0 (ESP8266)
- ESP32-S3 DevKit (Waveshare ESP32-S3-Touch-LCD-7)
- USB cables (3× mini/micro/USB-C)
- SHT31 sensor
- HX711 + load cell
- 3× SSR relays
- 4× tactile buttons (optional for Nano)
- Power supply (5V/3A minimum)
- Voltage divider resistors (10kΩ + 20kΩ)

### 1.2 Software Required

| Tool | Version | Purpose |
|------|---------|---------|
| Arduino IDE | 1.8.19+ | Firmware development |
| ESP32 Board Package | 2.0.x+ | ESP32/ESP32-S3 support |
| ESP8266 Board Package | 3.0.x+ | NodeMCU support |
| Arduino AVR Board Package | 1.8.x+ | Nano support |

## 2. Required Libraries

### 2.1 Arduino Library Manager

| Library | Author | Minimum Version |
|---------|--------|-----------------|
| **PID_v1** | Brett Beauregard | 1.2.1 |
| **Adafruit SHT31 Library** | Adafruit | 2.2.0 |
| **HX711 Arduino Library** | Bogdan Necula | 0.7.5 |
| **SoftwareWire** | — | — |
| **EspSoftwareSerial** | plerup | — |

### 2.2 ESP32-S3 Display Libraries

The HMIDisplay requires ESP Display Panel library. Install via Arduino Library Manager:
- Search for "ESP32_Display_Panel" by Espressif Systems
- Requires ESP32 Arduino v2.0.14+ or v3.x

## 3. PlatformIO Configuration

For advanced users, create `platformio.ini`:

```ini
[env:nano]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps =
    br3ttb/PID@^1.2.1
    adafruit/Adafruit SHT31 Library@^2.2.0
    bogde/HX711@^0.7.5
    plerup/SoftwareWire@^2.1.0
    plerup/EspSoftwareSerial@^8.1.0
monitor_speed = 115200

[env:nodemcu]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps =
    plerup/EspSoftwareSerial@^8.1.0
monitor_speed = 115200

[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps =
    br3ttb/PID@^1.2.1
    adafruit/Adafruit SHT31 Library@^2.2.0
    bogde/HX711@^0.7.5
    espressif/ESP32_Display_Panel@^1.2.0
monitor_speed = 115200
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_mode = qio
board_build.f_flash = 80000000L
```

## 4. Flashing Guide

### 4.1 Arduino Nano (`src/nano/FishDryer/`)

1. Open `FishDryer.ino` in Arduino IDE
2. Select **Board:** Arduino Nano
3. Select **Processor:** ATmega328P (Old Bootloader) if using a clone
4. Select **COM Port:** (your Nano port)
5. **Upload Speed:** 115200
6. Click **Upload**

### 4.2 NodeMCU Bridge (`src/esp/NodeMCUBridge/`)

1. Open `NodeMCUBridge.ino` in Arduino IDE
2. **Board:** NodeMCU 1.0 (ESP-12E Module)
3. **Flash Size:** 4MB (FS:2MB OTA:~1MB)
4. **Upload Speed:** 115200
5. Click **Upload**
6. Open Serial Monitor → copy MAC address

### 4.3 HMIDisplay (`src/esp/HMIDisplay/`)

1. Open `HMIDisplay.ino`
2. **Board:** ESP32S3 Dev Module
3. **USB CDC On Boot:** Enabled (for Serial output)
4. **PSRAM:** OPI PSRAM
5. **Flash Mode:** QIO
6. **Partition Scheme:** 16MB Flash (3MB APP/9.9MB FATFS)
7. Click **Upload**

**Note:** First upload may take 2-3 minutes due to PSRAM initialization.

### 4.4 Boot Sample (`src/sample/BootSample/`)

Standalone boot screen demo for testing the HMI display without sensors.

## 5. ESP-Now Pairing Sequence

```
Step 1: Flash NodeMCU Bridge (any MAC works)
        └─ Read its printed MAC from Serial Monitor
        └─ Expected: "[NodeMCU Bridge] MAC: XX:XX:XX:XX:XX:XX"

Step 2: Update serial_protocol.cpp in HMIDisplay
        └─ Set NODEMCU_PEER_MAC = NodeMCU's MAC

Step 3: Flash HMIDisplay
        └─ Read its printed MAC from Serial Monitor
        └─ Expected: "[HMI] NodeMCU peer MAC: XX:XX:XX:XX:XX:XX"
        └─ NOT the "copy to NodeMCUBridge" line

Step 4: Update NodeMCUBridge.ino
        └─ Set HMI_PEER_MAC = HMIDisplay's MAC

Step 5: Re-flash NodeMCU Bridge

Step 6: Verify — open both Serial Monitors
        └─ HMI should show "Connected"
        └─ NodeMCU should show successful ESP-Now send
```

## 6. Verification

After flashing all three nodes:

1. Open **Nano Serial Monitor** at 115200 baud
2. Press reset — should see:
   ```
   SSR_CONFIG loaded. SSR1_PIN, SSR2_PIN, and SSR3_PIN set as OUTPUT.
   [SHT31] SW I2C on D7/D8 ready
   Load cell: no saved calibration, using default factor...
   HX711 load cell ready (KG mode).
   PID controller initialized.
   Start/Stop button initialized on D11 (INPUT_PULLUP).
   READY  UART D9(RX)/D10(TX) baud=9600
   ```
3. Open **NodeMCU Serial Monitor** — should see periodic `STATUS?` polling
4. **HMI Display** should show boot screen → Dashboard with live data

## 7. Directory Structure for Development

```
fish-dryer-v2/
├── src/
│   ├── esp/
│   │   ├── FishDryer/          -- ESP32 controller
│   │   ├── HMIDisplay/         -- ESP32-S3 HMI (largest codebase)
│   │   └── NodeMCUBridge/      -- ESP8266 bridge
│   ├── nano/
│   │   └── FishDryer/          -- Arduino Nano controller (PRIMARY)
│   └── sample/
│       ├── BootSample/         -- Standalone boot screen
│       ├── NanoUartSample/     -- Nano UART test
│       └── NODEMCUUartSample/  -- NodeMCU UART test
├── wiring/                     -- Circuit diagrams
├── model/                      -- 3D renderings
└── docs/                       -- Documentation
```
