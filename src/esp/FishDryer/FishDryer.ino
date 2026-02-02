// FishDryer.ino
// ESP-based fish dryer controller

#include "SSR_CONFIG.h"
#include "SHT31_CONFIG.h"
#include "LOADCELL_CONFIG.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  // Add hardware initialization here

  pinMode(SSR1_PIN, OUTPUT);
  pinMode(SSR2_PIN, OUTPUT);
  Serial.println("SSR_CONFIG loaded. SSR1_PIN and SSR2_PIN set as OUTPUT.");

  Wire.begin();
  initSHT31();
  initLoadCell();
}

void loop() {
  // Main control loop
  // Listen for serial commands to control SSR
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "SSR1:1") {
      digitalWrite(SSR1_PIN, HIGH);
      Serial.println("SSR1_PIN set HIGH");
    } else if (cmd == "SSR1:0") {
      digitalWrite(SSR1_PIN, LOW);
      Serial.println("SSR1_PIN set LOW");
    } else if (cmd == "SSR2:1") {
      digitalWrite(SSR2_PIN, HIGH);
      Serial.println("SSR2_PIN set HIGH");
    } else if (cmd == "SSR2:0") {
      digitalWrite(SSR2_PIN, LOW);
      Serial.println("SSR2_PIN set LOW");
    } else if (cmd == "SHT31:READ") {
      readSHT31();
    } else if (cmd == "LOADCELL:READ") {
      readLoadCell();
    } else {
      Serial.println("Unknown command. Use SSR1:1, SSR1:0, SSR2:1, SSR2:0, SHT31:READ, or LOADCELL:READ.");
    }
  }
}
