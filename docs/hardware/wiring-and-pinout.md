# Hardware Wiring & Pinout

**Document Version:** 1.0.0  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The Fish Dryer v2 uses **two different microcontroller platforms** with distinct pin assignments. This document covers both the Arduino Nano (primary controller) and the ESP32 (alternative controller) along with the ESP32-S3 HMI display.

---

## 2. Arduino Nano Pin Assignments

The Nano is the **primary controller** running the drying process. Sensors are on Software I2C (D7/D8), UART to NodeMCU on D9/D10, and relays on D4-D6.

### 2.1 SSR / Relay Outputs

| SSR | Function | Nano Pin | Active Level |
|-----|----------|----------|-------------|
| SSR1 (Heating Element) | Heating element control | **D6** | HIGH = ON |
| SSR2 (Convection Fan) | Air circulation fan | **D4** | HIGH = ON |
| SSR3 (Exhaust Fan) | Moisture exhaust fan | **D5** | HIGH = ON |

### 2.2 Sensor Interfaces

| Sensor | Pins | Protocol | Notes |
|--------|------|----------|-------|
| SHT31 (Temp/Humidity) | **D7** (SDA), **D8** (SCL) | SoftwareWire (Software I2C) | Addr 0x44, high-repeatability |
| HX711 (Load Cell) | **D2** (DOUT), **D3** (SCK) | HX711 2-wire | 24-bit ADC |
| Physical Button | **D11** (INPUT_PULLUP) | Digital (active LOW) | START/STOP toggle |

### 2.3 UART to NodeMCU (SoftwareSerial)

| Nano Pin | Signal | Direction | Connects To |
|----------|--------|-----------|-------------|
| **D9** | SS_RX | ← Receive | NodeMCU D6 (GPIO12, TX) — 3.3V OK |
| **D10** | SS_TX | → Transmit | NodeMCU D5 (GPIO14, RX) — **Level shift required** ⚠ |

**Voltage Divider (Nano D10 → NodeMCU D5):**
```
Nano D10 (5V) ──[10 kΩ]──┬── NodeMCU D5 (3.3V max)
                          │
                       [20 kΩ]
                          │
                         GND
```

### 2.4 Complete Nano Pin Map

| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| D2 | HX711 DOUT | Input | Load cell data |
| D3 | HX711 SCK | Output | Load cell clock |
| D4 | SSR2 (Fan) | Output | Convection fan relay |
| D5 | SSR3 (Exhaust) | Output | Exhaust fan relay |
| D6 | SSR1 (Heater) | Output | Heating element relay |
| D7 | SHT31 SDA | I/O | Software I2C data |
| D8 | SHT31 SCL | Output | Software I2C clock |
| D9 | SS RX | Input | From NodeMCU TX |
| D10 | SS TX | Output | To NodeMCU RX (level shift!) |
| D11 | Button 5 | Input | Physical START/STOP (pullup) |
| A4/A5 | — | — | Freed (no I2C slave used) |

---

## 3. ESP32 Pin Assignments (38-Pin DevKit)

The ESP32 firmware (`src/esp/FishDryer/`) is an **alternative controller** with buttons.

### 3.1 SSR / Relay Outputs

| SSR | Function | GPIO | Active Level |
|-----|----------|------|-------------|
| SSR1 | Heating Element | **GPIO 16** | HIGH = ON |
| SSR2 | Convection Fan | **GPIO 17** | HIGH = ON |
| SSR3 | Exhaust Fan | **GPIO 18** | HIGH = ON |

### 3.2 Buttons (INPUT_PULLUP, Active LOW)

| Button | GPIO | Function |
|--------|------|----------|
| BUTTON1 | **GPIO 32** | User programmable (short/long press) |
| BUTTON2 | **GPIO 33** | User programmable |
| BUTTON3 | **GPIO 25** | User programmable |
| BUTTON4 | **GPIO 26** | User programmable |

### 3.3 I2C Bus (SHT31)

| Signal | GPIO | Notes |
|--------|------|-------|
| SDA | **GPIO 21** | I2C data (400kHz) |
| SCL | **GPIO 22** | I2C clock |

### 3.4 HX711 Load Cell

| Signal | GPIO | Direction |
|--------|------|-----------|
| DOUT | **GPIO 34** | Input (input-only GPIO OK) |
| SCK | **GPIO 27** | Output |

---

## 4. HMIDisplay (ESP32-S3 Waveshare 7")

### 4.1 LCD Interface (RGB-565, 16-bit)

| ESP32-S3 GPIO | RGB Signal | Notes |
|---------------|------------|-------|
| 14 | B0 | Data bit 0 |
| 38 | B1 | Data bit 1 |
| 18 | B2 | Data bit 2 |
| 17 | B3 | Data bit 3 |
| 10 | B4 | Data bit 4 |
| 39 | G0 | Data bit 5 |
| 0 | G1 | Data bit 6 |
| 45 | G2 | Data bit 7 |
| 48 | G3 | Data bit 8 |
| 47 | G4 | Data bit 9 |
| 21 | G5 | Data bit 10 |
| 1 | R0 | Data bit 11 |
| 2 | R1 | Data bit 12 |
| 42 | R2 | Data bit 13 |
| 41 | R3 | Data bit 14 |
| 40 | R4 | Data bit 15 |
| 46 | HSYNC | Horizontal sync |
| 3 | VSYNC | Vertical sync |
| 5 | DE | Data enable |
| 7 | PCLK | Pixel clock (16 MHz) |

### 4.2 Touch (GT911 — I2C)

| ESP32-S3 GPIO | Touch Signal |
|---------------|--------------|
| 9 | I2C SCL |
| 8 | I2C SDA |
| 4 | Interrupt (INT) |

### 4.3 IO Expander (CH422G — I2C)

| Pin | Function | Notes |
|-----|----------|-------|
| IO-0 | Backlight control | Switch on/off |
| IO-1 | Touch RST | Reset for GT911 |
| IO-3 | LCD RST | Reset for ST7262 |

### 4.4 Other

| ESP32-S3 GPIO | Function |
|---------------|----------|
| 46 | LCD HSYNC |
| 3 | LCD VSYNC |

---

## 5. NodeMCU (ESP8266) Pin Assignments

| NodeMCU Pin | GPIO | Function | Connects To |
|-------------|------|----------|-------------|
| D6 | GPIO12 | SS_TX → Nano D9 RX | UART transmit (3.3V → 5V OK) |
| D5 | GPIO14 | SS_RX ← Nano D10 TX | UART receive (5V → 3.3V **level shift!**) |
| — | — | ESP-Now antenna | To HMIDisplay (same Wi-Fi channel) |

---

## 6. Wiring Requirements

### 6.1 Power

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| Arduino Nano | 7-12V (VIN) or 5V (USB) | ~50mA | Via USB or external |
| NodeMCU | 5V (USB) or 3.3V (VIN) | ~80mA | USB-powered |
| ESP32-S3 HMI | 5V (USB-C) | ~500mA | Display backlight dominant |
| SHT31 | 3.3V | ~1.5mW | Powered from controller |
| HX711 | 5V (Nano) or 3.3V (ESP32) | ~10mW | Check VCC compatibility |
| SSR Input | 3.3-5V logic | ~10mA each | Isolated via optocoupler |

### 6.2 Critical Notes

1. **Nano D10 → NodeMCU D5 voltage divider is MANDATORY.** 5V will damage ESP8266 GPIO.
2. **SHT31 on Nano** uses SoftwareWire on D7/D8 — NOT the hardware I2C (A4/A5).
3. **HX711 DOUT** on ESP32 uses GPIO 34 (input-only) — no pullup needed.
4. **ESP32 pins 6-11** are flash-connected — DO NOT use for I/O.
5. **All SSR grounds** must be common with controller ground.

---

## 7. Wiring Diagram

The interactive wiring diagram is available on Cirkit Designer:

- **Web:** [https://app.cirkitdesigner.com/project/edca0d0e-6361-4e7a-a81a-6921ec4fce70](https://app.cirkitdesigner.com/project/edca0d0e-6361-4e7a-a81a-6921ec4fce70)
- **Source:** `wiring/Fish Dryer V2.ckt`
- **Image:** `wiring/circuit_image.png`
