
# Fish Dryer v2

A project for managing and monitoring a fish dryer system using ESP microcontroller.

## Features
- Modular code in `src/`
- SSR (Solid State Relay) control
- SHT31 temperature and humidity sensor integration
- HX711 load cell integration
- Serial commands for hardware testing
- Version-controlled with Git

## Hardware Modules
- **SSR Relays:**
	- **SSR1 (GPIO 16):** Heating Element
	- **SSR2 (GPIO 17):** Convection Fan
	- **SSR3 (GPIO 18):** Exhaust Fan
- **SHT31:** Read temperature and humidity
- **HX711:** Read load cell values

## Serial Commands
Send these commands via the serial monitor to test hardware:

- `SSR1:1` — Turn Heating Element ON
- `SSR1:0` — Turn Heating Element OFF
- `SSR2:1` — Turn Convection Fan ON
- `SSR2:0` — Turn Convection Fan OFF
- `SSR3:1` — Turn Exhaust Fan ON
- `SSR3:0` — Turn Exhaust Fan OFF
- `SHT31:READ` — Read temperature and humidity
- `LOADCELL:READ` — Read load cell value

## Getting Started
1. Clone the repository
2. Install dependencies (see relevant docs)
	- Arduino libraries: Adafruit_SHT31, HX711
3. Upload code to your ESP device
4. Use serial monitor for hardware testing

## License
See `LICENSE` for details.
