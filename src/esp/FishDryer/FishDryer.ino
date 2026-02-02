// FishDryer.ino
// ESP-based fish dryer controller

#include "SSR_CONFIG.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  // Add hardware initialization here

  pinMode(SSR_PIN, OUTPUT);
  Serial.println("SSR_CONFIG loaded. SSR_PIN set as OUTPUT.");
}

void loop() {
  // Main control loop
  // Listen for serial commands to control SSR
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "SSR:1") {
      digitalWrite(SSR_PIN, HIGH);
      Serial.println("SSR_PIN set HIGH");
    } else if (cmd == "SSR:0") {
      digitalWrite(SSR_PIN, LOW);
      Serial.println("SSR_PIN set LOW");
    } else {
      Serial.println("Unknown command. Use SSR:1 or SSR:0.");
    }
  }
}
