
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
- **SSR**: Control two relays via serial commands
- **SHT31**: Read temperature and humidity
- **HX711**: Read load cell values

## Serial Commands
Send these commands via the serial monitor to test hardware:

- `SSR1:1` — Turn SSR1 ON
- `SSR1:0` — Turn SSR1 OFF
- `SSR2:1` — Turn SSR2 ON
- `SSR2:0` — Turn SSR2 OFF
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
