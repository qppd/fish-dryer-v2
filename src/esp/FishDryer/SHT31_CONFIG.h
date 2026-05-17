// SHT31_CONFIG.h
// SHT31 temperature and humidity sensor integration

#ifndef SHT31_CONFIG_H
#define SHT31_CONFIG_H

#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void initSHT31() {
  if (!sht31.begin(0x44)) { // Default I2C address
    Serial.println("Couldn't find SHT31 sensor!");
  } else {
    Serial.println("SHT31 sensor initialized.");
  }
}

// Global sensor values for system-wide use
extern double CURRENT_TEMPERATURE;
extern double CURRENT_HUMIDITY;

void updateSensors() {
  float temp = sht31.readTemperature();
  float hum = sht31.readHumidity();

  if (!isnan(temp)) {
    CURRENT_TEMPERATURE = temp;
  }
  if (!isnan(hum)) {
    CURRENT_HUMIDITY = hum;
  }
}

void readSHT31() {
  updateSensors();
  Serial.print("SHT31 Temp: ");
  Serial.print(CURRENT_TEMPERATURE);
  Serial.print(" C, Humidity: ");
  Serial.print(CURRENT_HUMIDITY);
  Serial.println(" %");
}

#endif // SHT31_CONFIG_H
