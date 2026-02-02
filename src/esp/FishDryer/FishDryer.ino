// FishDryer.ino
// ESP-based fish dryer controller

#include "PINS_CONFIG.h"
#include "SSR_CONFIG.h" // SSR1: Heating Element, SSR2: Convection Fan, SSR3: Exhaust Fan
#include "SHT31_CONFIG.h"
#include "LOADCELL_CONFIG.h"
#include "BUTTON_CONFIG.h"
#include "PID_CONFIG.h"

// PID global variables
double CURRENT_TEMPERATURE = 0;
double TEMPERATURE_SETPOINT = 60.0; // Default setpoint in °C
bool PID_ENABLED = false; // PID control state
unsigned long lastPIDUpdate = 0;
const unsigned long PID_UPDATE_INTERVAL = 2000; // Update every 2 seconds

// Function to control SSR relays
void operateSSR(int relayIndex, bool state) {
  switch (relayIndex) {
    case 1:
      digitalWrite(SSR1_PIN, state ? HIGH : LOW);
      break;
    case 2:
      digitalWrite(SSR2_PIN, state ? HIGH : LOW);
      break;
    case 3:
      digitalWrite(SSR3_PIN, state ? HIGH : LOW);
      break;
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  // Add hardware initialization here

  pinMode(SSR1_PIN, OUTPUT);
  pinMode(SSR2_PIN, OUTPUT);
  pinMode(SSR3_PIN, OUTPUT);
  Serial.println("SSR_CONFIG loaded. SSR1_PIN, SSR2_PIN, and SSR3_PIN set as OUTPUT.");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  initSHT31();
  initLoadCell();
  initButtons();
  initPID();
  Serial.println("PID controller initialized.");
}

void loop() {
  // Update button states (non-blocking)
  updateButtons();
  
  // PID thermostat control (non-blocking)
  if (PID_ENABLED && (millis() - lastPIDUpdate >= PID_UPDATE_INTERVAL)) {
    updateTemperature(); // Read current temperature from SHT31
    pidCOMPUTE(); // Compute PID and update relays
    lastPIDUpdate = millis();
  }
  
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
    } else if (cmd == "SSR3:1") {
      digitalWrite(SSR3_PIN, HIGH);
      Serial.println("SSR3_PIN set HIGH");
    } else if (cmd == "SSR3:0") {
      digitalWrite(SSR3_PIN, LOW);
      Serial.println("SSR3_PIN set LOW");
    } else if (cmd == "SHT31:READ") {
      readSHT31();
    } else if (cmd == "LOADCELL:READ") {
      readLoadCell();
    } else if (cmd == "BUTTONS:READ") {
      readButtons();
    } else if (cmd == "PID:START") {
      PID_ENABLED = true;
      Serial.println("PID thermostat control ENABLED. System will maintain temperature automatically.");
    } else if (cmd == "PID:STOP") {
      PID_ENABLED = false;
      Serial.println("PID thermostat control DISABLED.");
    } else if (cmd.startsWith("PID:SET:")) {
      TEMPERATURE_SETPOINT = cmd.substring(8).toFloat();
      Serial.print("Temperature setpoint set to: ");
      Serial.println(TEMPERATURE_SETPOINT);
    } else if (cmd == "PID:READ") {
      Serial.print("PID Status: ");
      Serial.print(PID_ENABLED ? "ENABLED" : "DISABLED");
      Serial.print(" | Current Temp: ");
      Serial.print(CURRENT_TEMPERATURE);
      Serial.print(" | Setpoint: ");
      Serial.print(TEMPERATURE_SETPOINT);
      Serial.print(" | PID Output: ");
      Serial.println(PID_OUTPUT);
    } else {
      Serial.println("Unknown command. Use SSR1:1, SSR1:0, SSR2:1, SSR2:0, SSR3:1, SSR3:0, SHT31:READ, LOADCELL:READ, BUTTONS:READ, PID:START, PID:STOP, PID:SET:<value>, or PID:READ.");
    }
  }
}
