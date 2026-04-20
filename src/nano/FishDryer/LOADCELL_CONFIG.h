// LOADCELL_CONFIG.h
// HX711 load cell configuration and functions

#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

#include <EEPROM.h>
#include "PINS_CONFIG.h"
#include "HX711.h"

// ===========================================================
// Calibration factor — used only when no EEPROM value exists.
// After the first CALIBRATE:<kg> command the computed factor
// is saved to EEPROM and restored automatically on boot.
// ===========================================================
#define LOADCELL_CALIBRATION_FACTOR  1.0f   // default (uncalibrated)

// Number of samples averaged per reading
#define LOADCELL_SAMPLES  100

// EEPROM layout (2 bytes magic + 4 bytes float = 6 bytes total)
#define LOADCELL_EEPROM_MAGIC       0xBF
#define LOADCELL_EEPROM_MAGIC_ADDR  0
#define LOADCELL_EEPROM_FACTOR_ADDR 1   // float stored at byte 1-4

HX711 scale;

void initLoadCell() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Restore calibration factor from EEPROM if a valid save exists
  uint8_t magic;
  EEPROM.get(LOADCELL_EEPROM_MAGIC_ADDR, magic);
  if (magic == LOADCELL_EEPROM_MAGIC) {
    float savedFactor;
    EEPROM.get(LOADCELL_EEPROM_FACTOR_ADDR, savedFactor);
    if (savedFactor > 0.0f) {
      scale.set_scale(savedFactor);
      Serial.print(F("Load cell: restored calibration factor from EEPROM: "));
      Serial.println(savedFactor, 6);
    } else {
      scale.set_scale(LOADCELL_CALIBRATION_FACTOR);
      Serial.println(F("Load cell: EEPROM factor invalid, using default."));
    }
  } else {
    scale.set_scale(LOADCELL_CALIBRATION_FACTOR);
    Serial.println(F("Load cell: no saved calibration, using default factor 1.0 (not kg!)."));
    Serial.println(F("  Send TARE then CALIBRATE:<kg> to calibrate."));
  }

  scale.tare();
  Serial.println(F("HX711 load cell ready (KG mode)."));
  Serial.println(F("  -> TARE              zero the scale"));
  Serial.println(F("  -> CALIBRATE:<kg>    calibrate with known weight"));
}

// Zero the scale (call with nothing on it)
void tareLoadCell() {
  if (!scale.is_ready()) { Serial.println(F("HX711 not ready!")); return; }
  scale.tare();
  Serial.println(F("Load cell tared. Place known weight, then send CALIBRATE:<kg>"));
}

// Run calibration against a known weight in KG.
// Step 1: send TARE         (empty scale)
// Step 2: place known weight
// Step 3: send CALIBRATE:1.0  (or whatever your reference weight is)
// The computed factor is saved to EEPROM and applied immediately.
void calibrateLoadCell(float known_kg) {
  scale.calibrate_scale(known_kg, LOADCELL_SAMPLES);
  float factor = scale.get_scale();

  // Persist to EEPROM so it survives power cycles
  EEPROM.put(LOADCELL_EEPROM_MAGIC_ADDR, (uint8_t)LOADCELL_EEPROM_MAGIC);
  EEPROM.put(LOADCELL_EEPROM_FACTOR_ADDR, factor);

  Serial.println(F("--- Calibration complete ---"));
  Serial.print(  F("Calibration factor: "));
  Serial.println(factor, 6);
  Serial.println(F("Factor saved to EEPROM. Will be restored on next power-up."));
}

// Returns weight in KG. Returns 0 if sensor not ready.
float readLoadCell() {
  if (!scale.is_ready()) {
    Serial.println(F("HX711 not ready!"));
    return 0.0f;
  }
  float kg = scale.get_units(LOADCELL_SAMPLES);
  if (kg < 0) kg = 0.0f;   // clamp negative noise
  return kg;
}

#endif // LOADCELL_CONFIG_H
