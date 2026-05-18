# Changelog

**Author:** Sajed Lopez Mendoza — QPPD

---

## v2.0.0 (Current)

### Features
- Modular configuration architecture (PINS_CONFIG.h, PID_CONFIG.h, etc.)
- PID temperature control (Kp=4.0, Ki=0.0, Kd=22.0)
- Load cell integration (HX711, EEPROM-persistent calibration)
- Button input system (debounce + short/long press)
- Serial command interface (same on both ESP32 and Nano)
- HMI Touchscreen Interface (LVGL v8, Waveshare 7" display)
- Dashboard with real-time temperature gauge
- Control panel with preset/manual mode selection
- Diagnostics and sensor status monitoring
- Scale calibration wizard modal
- Estimated Drying Time (EDT) calculation
- Water loss progress bar
- WiFi/ESP-Now connectivity to controller
- Static 3-second boot screen using compiled `solaraw` C-array image
- Cross-screen optimistic START/STOP UI sync (15s grace)
- Drying presets (Tuyo 60°C, Danggit 60°C, Pusit 50°C)
- Arduino Nano firmware (non-blocking SHT31 state machine, SoftwareWire)
- NodeMCU UART↔ESP-Now bridge firmware
- EEPROM-based calibration persistence with validation

### System Architecture
- 3-node architecture: Nano (controller) ↔ NodeMCU (bridge) ↔ ESP32-S3 (HMI)
- ESP-Now binary packet protocol (38B status, 7B command)
- UART text protocol (9600 baud, JSON status)
- Non-blocking millis()-based timing throughout

### Safety
- SHT31 error detection with graceful degradation
- Connection timeout detection (6s)
- Temperature alerts (>95°C warning, >110°C emergency)
- Drying complete detection and alert

## Planned v2.1.0

- SD card data logging
- Battery voltage monitoring
- Cloud connectivity for remote monitoring
- Mobile app integration
