# Fish Dryer v2

**An intelligent, solar-ready fish drying control system.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-orange)](https://www.espressif.com/)
[![Version: 2.0.0](https://img.shields.io/badge/Version-2.0.0-brightgreen)]()
[![Author: QPPD](https://img.shields.io/badge/Author-QPPD-ff69b4)]()

---

## Overview

Fish Dryer v2 is a multi-microcontroller industrial control system that automates fish drying using PID-based temperature regulation, real-time weight monitoring, and an LVGL-powered touchscreen HMI. Designed for off-grid operation with solar power integration.

## Core Features

- **PID Temperature Control** — Closed-loop regulation (Kp=4.0, Ki=0.0, Kd=22.0)
- **Weight-Based Automation** — HX711 load cell with auto-stop at target water loss
- **Multi-Node Architecture** — Arduino Nano (controller) + NodeMCU (bridge) + ESP32-S3 (HMI)
- **HMI Touchscreen** — 7" LVGL dashboard with real-time gauges, charts, and preset profiles
- **ESP-Now Wireless** — Binary packet protocol between bridge and display
- **Solar-Ready Power** — Designed for 600W solar + 300Ah LiFePO4 battery system
- **Safety-First** — Sensor failure detection, overheat cutoff, graceful shutdown

## Tech Stack

| Component           | Technology                                         |
|---------------------|----------------------------------------------------|
| Controller          | ESP32 (Arduino) / Arduino Nano (ATmega328P)        |
| HMI Display         | ESP32-S3 + LVGL v8 + Waveshare 7" RGB LCD          |
| Communication       | ESP-Now (binary) + UART (JSON/text)                |
| Sensors             | SHT31 (temp/humidity), HX711 (load cell)           |
| Actuators           | 3x SSR relays (heater, fan, exhaust)               |
| UI Framework        | LVGL v8.3+ (Montserrat fonts, gauge, charts)       |

## Quick Start

```bash
git clone https://github.com/qppd/fish-dryer-v2.git
cd fish-dryer-v2
```

Open `src/nano/FishDryer/FishDryer.ino` in Arduino IDE (Board: Arduino Nano).  
Open `src/esp/NodeMCUBridge/NodeMCUBridge.ino` (Board: NodeMCU 1.0).  
Open `src/esp/HMIDisplay/HMIDisplay.ino` (Board: ESP32-S3 via ESP Display Panel).

## Documentation

Full system documentation is in the **[docs/](docs/)** directory:

- [System Architecture](docs/architecture/system-architecture.md)
- [Firmware Documentation](docs/firmware/)
- [Hardware & Wiring](docs/hardware/)
- [Communication Protocols](docs/apis/)
- [HMI / Frontend](docs/frontend/)
- [Calculations & Algorithms](docs/calculations/)
- [System Flows](docs/flows/)
- [Setup & Deployment](docs/setup/)
- [Troubleshooting](docs/troubleshooting/)
- [Testing](docs/testing/)

## Project Structure

```
src/
  esp/
    FishDryer/           -- ESP32 controller firmware (main firmware)
    HMIDisplay/          -- ESP32-S3 HMI with LVGL
    NodeMCUBridge/       -- ESP8266 ESP-Now ↔ UART bridge
  nano/
    FishDryer/           -- Arduino Nano controller firmware
  sample/                -- Standalone test sketches
wiring/                  -- Circuit diagram (Cirkit Designer)
model/                   -- 3D enclosure renderings
lvgl/                    -- LVGL porting samples
docs/                    -- Full system documentation
```

## Author

**Sajed Lopez Mendoza**  
Organization: **QPPD**  
- Email: quezon.province.pd@gmail.com  
- GitHub: [@qppd](https://github.com/qppd)  
- Portfolio: [sajed-mendoza.onrender.com](https://sajed-mendoza.onrender.com)

## License

MIT License — see [LICENSE](LICENSE).