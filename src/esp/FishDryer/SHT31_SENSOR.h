// SHT31_SENSOR.h
// SHT31 temperature and humidity sensor integration

#ifndef SHT31_SENSOR_H
#define SHT31_SENSOR_H

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

void readSHT31() {
  float temp = sht31.readTemperature();
  float humidity = sht31.readHumidity();
  if (!isnan(temp) && !isnan(humidity)) {
    Serial.print("SHT31 Temp: ");
    Serial.print(temp);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  } else {
    Serial.println("Failed to read SHT31 sensor!");
  }
}

#endif // SHT31_SENSOR_H
