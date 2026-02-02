
# Fish Dryer v2

## Overview
Fish Dryer v2 is a modular, open-source firmware for an ESP32-based fish dryer controller. It provides robust control and monitoring of heating, convection, and exhaust systems, as well as environmental sensing and user input, all with a focus on safety and extensibility.

## Architecture
The project is organized around a single Arduino sketch (`FishDryer.ino`) and a set of configuration headers for each hardware module. All pin assignments are centralized in `PINS_CONFIG.h` for clarity and maintainability. Each module (relays, sensors, buttons) is initialized and managed independently, with non-blocking logic for user input and serial command handling for testing and integration.

## Features
- Three solid-state relay (SSR) outputs for heating, convection fan, and exhaust fan
- SHT31 temperature and humidity sensor integration (I2C)
- HX711 load cell integration for weight measurement
- Four tactile buttons with software debounce, short/long press detection, and non-blocking logic
- Serial command interface for hardware testing and diagnostics
- Modular configuration headers for easy hardware adaptation
- ESP32 38-pin board compatibility with safe default GPIO assignments

## Hardware Requirements
- ESP32 38-pin development board (with mounting holes recommended)
- Three SSRs or relay modules (rated for your load)
- SHT31 temperature and humidity sensor (I2C)
- HX711 load cell amplifier and compatible load cell
- Four tactile push buttons (normally open)
- Power supply suitable for ESP32 and all peripherals
- Wiring, connectors, and (for mains) proper enclosures and safety equipment

## Software Components
- Arduino sketch: `FishDryer.ino`
- Configuration headers: `PINS_CONFIG.h`, `SSR_CONFIG.h`, `SHT31_CONFIG.h`, `LOADCELL_CONFIG.h`, `BUTTON_CONFIG.h`
- Arduino libraries: [Adafruit SHT31](https://github.com/adafruit/Adafruit_SHT31), [HX711](https://github.com/bogde/HX711)

## Installation
1. Clone this repository:
   ```sh
   git clone https://github.com/qppd/fish-dryer-v2.git
   ```
2. Open `src/esp/FishDryer/FishDryer.ino` in the Arduino IDE.
3. Install the required libraries via Library Manager:
   - Adafruit SHT31
   - HX711
4. Select your ESP32 board and COM port.
5. Compile and upload the firmware.

## Usage
After uploading, open the Serial Monitor at 115200 baud. Use the following serial commands to test and control hardware:

### Relay Control
| Command    | Function                |
|------------|-------------------------|
| SSR1:1     | Heating Element ON      |
| SSR1:0     | Heating Element OFF     |
| SSR2:1     | Convection Fan ON       |
| SSR2:0     | Convection Fan OFF      |
| SSR3:1     | Exhaust Fan ON          |
| SSR3:0     | Exhaust Fan OFF         |

### Sensor Readings
| Command        | Function                        |
|---------------|---------------------------------|
| SHT31:READ    | Print temperature and humidity  |
| LOADCELL:READ | Print load cell raw value       |

### Button State
| Command        | Function                        |
|---------------|---------------------------------|
| BUTTONS:READ  | Print all button states         |

Button presses (short/long) are also reported to the serial monitor in real time.

## Hardware Models

### Pin Assignments (see `PINS_CONFIG.h`)

#### Relays
| Channel | Function           | Config Symbol | Default GPIO |
|--------:|--------------------|--------------|-------------:|
| SSR1    | Heating Element    | SSR1_PIN     | 16           |
| SSR2    | Convection Fan     | SSR2_PIN     | 17           |
| SSR3    | Exhaust Fan        | SSR3_PIN     | 18           |

#### Buttons
| Button | Config Symbol | Default GPIO | Notes                  |
|-------:|---------------|-------------:|------------------------|
| BTN1   | BUTTON1_PIN   | 32           | INPUT_PULLUP, active-low|
| BTN2   | BUTTON2_PIN   | 33           | INPUT_PULLUP, active-low|
| BTN3   | BUTTON3_PIN   | 25           | INPUT_PULLUP, active-low|
| BTN4   | BUTTON4_PIN   | 26           | INPUT_PULLUP, active-low|

#### HX711
| Signal | Config Symbol         | Default GPIO | Notes |
|--------|-----------------------|-------------:|-------|
| DOUT   | LOADCELL_DOUT_PIN     | 34           | Input-only GPIO is fine |
| SCK    | LOADCELL_SCK_PIN      | 27           | Output GPIO            |

#### I2C (SHT31)
| Signal | Config Symbol | Default GPIO |
|--------|--------------|-------------:|
| SDA    | I2C_SDA_PIN  | 21           |
| SCL    | I2C_SCL_PIN  | 22           |

## Project Structure
```
src/
  esp/
    FishDryer/
      FishDryer.ino
      PINS_CONFIG.h
      SSR_CONFIG.h
      SHT31_CONFIG.h
      LOADCELL_CONFIG.h
      BUTTON_CONFIG.h
```

## Configuration
All hardware pin assignments are set in `PINS_CONFIG.h`. Each module header includes this file and uses the defined symbols. Adjust these values to match your custom hardware if needed.

## Development
- All button handling is non-blocking and uses `millis()` for debounce and press duration.
- Short and long press actions are implemented in `handleShortPress()` and `handleLongPress()` in `BUTTON_CONFIG.h`.
- Serial commands are parsed in the main loop for easy hardware testing and integration.
- The code is modular and easy to extend for additional sensors or actuators.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request. For major changes, open an issue first to discuss your proposal.

## License
This project is licensed under the terms of the MIT License. See the `LICENSE` file for details.
