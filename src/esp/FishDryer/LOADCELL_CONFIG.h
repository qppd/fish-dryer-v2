// LOADCELL_CONFIG.h
// HX711 load cell configuration and functions

#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

#include "PINS_CONFIG.h"
#include "HX711.h"

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
