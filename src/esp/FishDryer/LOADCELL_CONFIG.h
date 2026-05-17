// LOADCELL_CONFIG.h
// HX711 load cell configuration and functions

#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

#include "PINS_CONFIG.h"
#include "HX711.h"

// Fixed calibration factor provided by user
const float LOADCELL_CALIBRATION_FACTOR = -10697.956054f;

HX711 scale;

void initLoadCell() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(LOADCELL_CALIBRATION_FACTOR);
  scale.tare(); // Reset scale to 0 at boot
  Serial.println("HX711 load cell initialized and tared.");
}

float getWeightKg() {
  if (scale.is_ready()) {
    // Read average of 5 samples for stability
    return scale.get_units(5);
  }
  return -1.0f; // Error value
}

void tareLoadCell() {
  if (scale.is_ready()) {
    scale.tare();
    Serial.println("Load cell tared successfully.");
  } else {
    Serial.println("HX711 not ready for tare!");
  }
}

void readLoadCell() {
  float weight = getWeightKg();
  if (weight >= 0) {
    Serial.print("Load cell weight: ");
    Serial.print(weight, 3);
    Serial.println(" kg");
  } else {
    Serial.println("HX711 not ready!");
  }
}

#endif // LOADCELL_CONFIG_H
