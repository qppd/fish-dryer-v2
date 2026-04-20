// PINS_CONFIG.h
// Centralized pin assignments for all hardware modules (Arduino Nano)
//
// Notes:
// - Nano digital pins: D2-D13; analog: A0-A7
// - A4/A5 are now used for SoftwareSerial UART to NodeMCU Bridge (Wire not used)

#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

// =========================
// SSR / Relay Outputs
// =========================
// SSR1: Heating Element
// SSR2: Convection Fan
// SSR3: Exhaust Fan
#ifndef SSR1_PIN
#define SSR1_PIN 6
#endif

#ifndef SSR2_PIN
#define SSR2_PIN 4
#endif

#ifndef SSR3_PIN
#define SSR3_PIN 5
#endif

// =========================
// SHT31 — Software I2C bus (D7/D8)
// Separate from hardware TWI; A4/A5 are now freed (no I2C slave)
// =========================
#ifndef SOFT_SDA_PIN
#define SOFT_SDA_PIN 7      // D7 — SHT31 SDA (SoftwareWire)
#endif

#ifndef SOFT_SCL_PIN
#define SOFT_SCL_PIN 8      // D8 — SHT31 SCL (SoftwareWire)
#endif

// =========================
// SoftwareSerial — UART to NodeMCU Bridge
//
// Wiring:
//   Nano D9  (RX) <-- NodeMCU D6 / GPIO12 (TX)   3.3 V → 5 V : OK
//   Nano D10 (TX) --> NodeMCU D5 / GPIO14 (RX)   5 V  → 3.3 V : use voltage divider
//     Nano D10 --[10kΩ]--+-- NodeMCU D5
//                         |
//                       [20kΩ]
//                         |
//                         GND
// =========================
#ifndef SS_RX_PIN
#define SS_RX_PIN   9       // SoftwareSerial RX ← NodeMCU TX (D6/GPIO12)
#endif

#ifndef SS_TX_PIN
#define SS_TX_PIN   10      // SoftwareSerial TX → NodeMCU RX (D5/GPIO14)
#endif

#ifndef SS_UART_BAUD
#define SS_UART_BAUD 9600
#endif

// =========================
// HX711 Load Cell
// =========================
// DOUT can be on an input-only pin.
#ifndef LOADCELL_DOUT_PIN
#define LOADCELL_DOUT_PIN 2
#endif

#ifndef LOADCELL_SCK_PIN
#define LOADCELL_SCK_PIN 3
#endif

#endif // PINS_CONFIG_H
