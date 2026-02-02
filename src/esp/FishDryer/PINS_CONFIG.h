// PINS_CONFIG.h
// Centralized pin assignments for all hardware modules (ESP32 38-pin board)
//
// Notes:
// - Avoid ESP32 flash pins GPIO 6-11.
// - Keep boot/strapping behavior in mind when changing pins.

#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

// =========================
// SSR / Relay Outputs
// =========================
// SSR1: Heating Element
// SSR2: Convection Fan
// SSR3: Exhaust Fan
#ifndef SSR1_PIN
#define SSR1_PIN 16
#endif

#ifndef SSR2_PIN
#define SSR2_PIN 17
#endif

#ifndef SSR3_PIN
#define SSR3_PIN 18
#endif

// =========================
// Tactile Buttons (INPUT_PULLUP, active-low)
// =========================
#ifndef BUTTON1_PIN
#define BUTTON1_PIN 32
#endif

#ifndef BUTTON2_PIN
#define BUTTON2_PIN 33
#endif

#ifndef BUTTON3_PIN
#define BUTTON3_PIN 25
#endif

#ifndef BUTTON4_PIN
#define BUTTON4_PIN 26
#endif

// =========================
// SHT31 (I2C)
// =========================
// Default ESP32 I2C pins (can be overridden)
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif

// =========================
// HX711 Load Cell
// =========================
// DOUT can be on an input-only pin.
#ifndef LOADCELL_DOUT_PIN
#define LOADCELL_DOUT_PIN 34
#endif

#ifndef LOADCELL_SCK_PIN
#define LOADCELL_SCK_PIN 27
#endif

#endif // PINS_CONFIG_H
