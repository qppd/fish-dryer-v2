# Documentation Index

**Fish Dryer v2** — Full system documentation

---

## Architecture
- [System Architecture](architecture/system-architecture.md) — Multi-node design, data flow, state machine, technology stack

## Firmware
- [ESP32 Controller](firmware/esp32-controller.md) — FishDryer.ino for ESP32
- [Arduino Nano Controller](firmware/nano-controller.md) — Nano FishDryer.ino
- [NodeMCU Bridge](firmware/nodemcu-bridge.md) — ESP-Now ↔ UART bridge
- [HMIDisplay Firmware](firmware/hmi-display.md) — LVGL touchscreen HMI

## Hardware
- [Wiring & Pinout](hardware/wiring-and-pinout.md) — Complete pin mapping for all nodes
- [Sensor Integration](hardware/sensors.md) — SHT31 and HX711 detailed analysis
- [Enclosure & 3D Model](hardware/enclosure.md) — Physical design references

## Communication / APIs
- [ESP-Now Binary Protocol](apis/esp-now-protocol.md) — Packet structures, commands, checksums
- [UART Text Protocol](apis/uart-protocol.md) — JSON and command formats
- [I2C Protocol (Legacy)](apis/i2c-protocol.md) — Reference I2C slave implementation

## Frontend (HMI)
- [HMI Screens](frontend/hmi-screens.md) — All LVGL screen implementations
- [UI Theme & Styling](frontend/ui-theme.md) — Colors, fonts, layout constants
- [Optimistic UI System](frontend/optimistic-ui.md) — Cross-screen START/STOP sync

## Calculations & Algorithms
- [Water Loss Calculation](calculations/water-loss.md) — Weight-based moisture tracking
- [PID Temperature Control](calculations/pid-control.md) — Tuning, limits, relay logic
- [EDT Estimation](calculations/estimated-drying-time.md) — Time-to-completion algorithm
- [SHT31 CRC-8 Calculation](calculations/sht31-crc.md) — CRC polynomial 0x31 implementation

## System Flows
- [Drying Process Flow](flows/drying-process.md) — Complete cycle from start to stop
- [Startup Sequence](flows/startup.md) — Power-on initialization across all nodes
- [Runtime Loop](flows/runtime-loop.md) — Non-blocking execution cycle
- [Emergency & Safety](flows/emergency.md) — Sensor failure, overheat, disconnection

## Setup
- [Development Environment](setup/development.md) — Arduino IDE, libraries, PlatformIO
- [Flashing Guide](setup/flashing.md) — How to flash each node
- [ESP-Now Pairing](setup/espnow-pairing.md) — MAC address exchange procedure

## Troubleshooting
- [SHT31 Issues](troubleshooting/sht31.md) — Sensor not found, CRC errors
- [Load Cell Issues](troubleshooting/loadcell.md) — Erratic readings, calibration
- [PID Issues](troubleshooting/pid.md) — Oscillation, overshoot, slow response
- [Communication Issues](troubleshooting/communication.md) — ESP-Now, UART dropout
- [HMI Issues](troubleshooting/hmi.md) — Display, touch, LVGL crashes

## Deployment
- [Solar Power System](deployment/solar-power.md) — Solar, battery, inverter specs
- [Production Checklist](deployment/checklist.md) — Pre-deployment verification

## Testing
- [Test Methodology](testing/methodology.md) — Approaches for testing each subsystem
- [Sample Sketches](testing/samples.md) — Standalone test sketches reference

## Reference
- [Project History](history/CHANGELOG.md) — Development changelog
- [Glossary](reference/glossary.md) — Terms and abbreviations
