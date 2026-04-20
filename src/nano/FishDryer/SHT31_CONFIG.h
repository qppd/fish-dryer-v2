// SHT31_CONFIG.h
// Non-blocking SHT31 reader via SoftwareWire on SOFT_SDA_PIN/SOFT_SCL_PIN (D7/D8)
//
// Two separate I2C buses on the Nano:
//   Hardware Wire (A4/A5) — slave 0x08, talks to ESP32 master
//   SoftwareWire (D7/D8)  — master, reads SHT31 at 0x44
//
// Call initSHT31()  once in setup().
// Call updateSHT31() every loop() iteration (non-blocking state machine).
// CURRENT_TEMPERATURE and CURRENT_HUMIDITY are updated automatically.

#ifndef SHT31_CONFIG_H
#define SHT31_CONFIG_H

#include <SoftwareWire.h>
#include "PINS_CONFIG.h"

// ── SHT31 constants ───────────────────────────────────────────────────────────
#define SHT31_ADDR              0x44    // change to 0x45 if ADDR pin is tied HIGH
#define SHT31_READ_INTERVAL_MS  2000    // sensor poll rate
#define SHT31_MEAS_DELAY_MS     20      // high-repeatability measurement time

// Single-shot command: high repeatability, clock-stretching disabled
#define SHT31_CMD_MSB           0x24
#define SHT31_CMD_LSB           0x00

// Externals defined in FishDryer.ino
extern bool   sht31OK;
extern float  CURRENT_HUMIDITY;
extern double CURRENT_TEMPERATURE;

// ── Software I2C bus (D7/D8, internal pull-ups enabled) ───────────────────────
static SoftwareWire sWire(SOFT_SDA_PIN, SOFT_SCL_PIN, true);

// ── State machine ─────────────────────────────────────────────────────────────
enum SHT31Phase : uint8_t { SHT_IDLE, SHT_WAITING };

static SHT31Phase    _sht31Phase      = SHT_IDLE;
static unsigned long _sht31LastRead   = 0;
static unsigned long _sht31TriggerMs  = 0;
static uint8_t       _sht31Fails      = 0;

// ── CRC-8 (polynomial 0x31, init 0xFF) ───────────────────────────────────────
static uint8_t _sht31CRC(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
  }
  return crc;
}

// ── Init ──────────────────────────────────────────────────────────────────────
void initSHT31() {
  sWire.begin();
  sht31OK = false;
  Serial.print(F("[SHT31] SW I2C on D"));
  Serial.print(SOFT_SDA_PIN);
  Serial.print(F("/D"));
  Serial.print(SOFT_SCL_PIN);
  Serial.println(F(" ready"));
}

// ── Non-blocking state machine — call every loop() ───────────────────────────
void updateSHT31() {
  unsigned long now = millis();

  switch (_sht31Phase) {

    case SHT_IDLE:
      if (now - _sht31LastRead < SHT31_READ_INTERVAL_MS) return;
      sWire.beginTransmission(SHT31_ADDR);
      sWire.write(SHT31_CMD_MSB);
      sWire.write(SHT31_CMD_LSB);
      if (sWire.endTransmission() == 0) {
        _sht31Phase     = SHT_WAITING;
        _sht31TriggerMs = now;
      } else {
        _sht31Fails++;
        _sht31LastRead = now;
        noInterrupts(); sht31OK = false; interrupts();
      }
      break;

    case SHT_WAITING:
      if (now - _sht31TriggerMs < SHT31_MEAS_DELAY_MS) return;
      _sht31LastRead = now;
      _sht31Phase    = SHT_IDLE;

      if (sWire.requestFrom((uint8_t)SHT31_ADDR, (uint8_t)6) != 6) {
        _sht31Fails++;
        noInterrupts(); sht31OK = false; interrupts();
        return;
      }
      {
        uint8_t buf[6];
        for (uint8_t i = 0; i < 6; i++) buf[i] = sWire.read();

        if (_sht31CRC(buf, 2) != buf[2] || _sht31CRC(buf + 3, 2) != buf[5]) {
          _sht31Fails++;
          noInterrupts(); sht31OK = false; interrupts();
          return;
        }

        uint16_t rawT = ((uint16_t)buf[0] << 8) | buf[1];
        uint16_t rawH = ((uint16_t)buf[3] << 8) | buf[4];
        float temp = -45.0f + 175.0f * ((float)rawT / 65535.0f);
        float hum  = 100.0f * ((float)rawH / 65535.0f);

        if (temp >= -40.0f && temp <= 125.0f && hum >= 0.0f && hum <= 100.0f) {
          noInterrupts();
          CURRENT_TEMPERATURE = (double)temp;
          CURRENT_HUMIDITY    = hum;
          sht31OK             = true;
          interrupts();
          _sht31Fails = 0;
        } else {
          _sht31Fails++;
          noInterrupts(); sht31OK = false; interrupts();
        }
      }
      break;
  }
}

// ── Compatibility stubs (called from FishDryer.ino) ──────────────────────────
void updateTemperature() { /* values updated by updateSHT31() state machine */ }
float getHumidity()      { return CURRENT_HUMIDITY; }
void readSHT31()         { /* state machine handles reading */ }

#endif // SHT31_CONFIG_H
