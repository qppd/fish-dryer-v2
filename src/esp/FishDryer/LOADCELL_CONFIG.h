// LOADCELL_CONFIG.h
// HX711 load cell configuration and functions

#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

#include "HX711.h"

#define LOADCELL_DOUT_PIN 34 // HX711 DOUT (ESP32 input-only GPIO is fine here)
#define LOADCELL_SCK_PIN 27  // HX711 SCK (ESP32 safe output GPIO)

HX711 scale;

void initLoadCell() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("HX711 load cell initialized.");
}

void readLoadCell() {
  if (scale.is_ready()) {
    long reading = scale.read();
    Serial.print("Load cell reading: ");
    Serial.println(reading);
  } else {
    Serial.println("HX711 not ready!");
  }
}

#endif // LOADCELL_CONFIG_H
