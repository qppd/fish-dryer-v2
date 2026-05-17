// FishDryer.ino
// ESP-based fish dryer controller

#include "PINS_CONFIG.h"
#include "SSR_CONFIG.h"
#include "SHT31_CONFIG.h"
#include "LOADCELL_CONFIG.h"
#include "BUTTON_CONFIG.h"
#include "PID_CONFIG.h"

// Global state variables
double CURRENT_TEMPERATURE = 0;
double CURRENT_HUMIDITY = 0;
float CURRENT_WEIGHT = 0;
float INITIAL_WEIGHT = 0;
float WATER_LOSS = 0;
float WATER_LOSS_TARGET = 15.0;

double TEMPERATURE_SETPOINT = 60.0;
bool PID_ENABLED = false;
unsigned long lastPIDUpdate = 0;
const unsigned long PID_UPDATE_INTERVAL = 2000;

// Drying state management
enum DryerState { IDLE, DRYING, COMPLETE, PAUSED };
DryerState systemState = IDLE;
unsigned long dryingStartTime = 0;
unsigned long lastSensorUpdate = 0;
const unsigned long SENSOR_UPDATE_INTERVAL = 2000;

// EDT Estimation
unsigned long lastEDTWindowStart = 0;
float weightAtWindowStart = 0;
float currentDryingRate = 0; // kg per second
long estimatedEDTSeconds = 0;
const unsigned long EDT_WINDOW_MS = 600000; // 10 minutes for rate calculation

// Auto-stop stability
int humidityStopCounter = 0;
const int HUMIDITY_STOP_THRESHOLD = 5; // Must be <= 14% for 5 * SENSOR_UPDATE_INTERVAL

void operateSSR(int relayIndex, bool state) {
  switch (relayIndex) {
    case 1: digitalWrite(SSR1_PIN, state ? HIGH : LOW); break;
    case 2: digitalWrite(SSR2_PIN, state ? HIGH : LOW); break;
    case 3: digitalWrite(SSR3_PIN, state ? HIGH : LOW); break;
  }
}

void stopDrying() {
  systemState = COMPLETE;
  PID_ENABLED = false;
  operateSSR(1, false);
  operateSSR(2, false);
  operateSSR(3, false);
  Serial.println("[AUTO-STOP] Humidity threshold 14% reached. System stopped.");
}

void startDrying() {
  INITIAL_WEIGHT = CURRENT_WEIGHT;
  if (INITIAL_WEIGHT <= 0) {
    Serial.println("[WARN] Starting drying with zero/invalid initial weight!");
  }
  systemState = DRYING;
  PID_ENABLED = true;
  dryingStartTime = millis();
  lastEDTWindowStart = millis();
  weightAtWindowStart = CURRENT_WEIGHT;
  humidityStopCounter = 0;
  Serial.println("[SYSTEM] Drying started. Initial weight captured.");
}

void sendStatusJSON() {
  String stateStr = "IDLE";
  if (systemState == DRYING) stateStr = "DRYING";
  else if (systemState == COMPLETE) stateStr = "COMPLETE";
  else if (systemState == PAUSED) stateStr = "PAUSED";

  unsigned long runtime = (systemState == DRYING || systemState == COMPLETE) ? (millis() - dryingStartTime) : 0;

  char jsonBuf[256];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"state\":\"%s\",\"temp\":%.2f,\"hum\":%.2f,\"weight\":%.3f,\"loss\":%.2f,\"target_temp\":%.2f,\"target_loss\":%.2f,\"pid_out\":%.2f,\"runtime\":%lu,\"heater\":%d,\"fan\":%d,\"exhaust\":%d,\"pid\":%d,\"sht31\":%d}",
    stateStr.c_str(), CURRENT_TEMPERATURE, CURRENT_HUMIDITY, CURRENT_WEIGHT, WATER_LOSS,
    TEMPERATURE_SETPOINT, WATER_LOSS_TARGET, PID_OUTPUT, runtime / 1000,
    digitalRead(SSR1_PIN), digitalRead(SSR2_PIN), digitalRead(SSR3_PIN),
    PID_ENABLED ? 1 : 0, (CURRENT_HUMIDITY > 0) ? 1 : 0);

  Serial.println(jsonBuf);
}

void updateDryingProcess() {
  // 1. Update Sensors
  updateSensors();
  CURRENT_WEIGHT = getWeightKg();
  if (CURRENT_WEIGHT < 0) CURRENT_WEIGHT = 0;

  if (systemState == DRYING) {
    // 2. Water Loss Computation
    if (INITIAL_WEIGHT > 0) {
      WATER_LOSS = ((INITIAL_WEIGHT - CURRENT_WEIGHT) / INITIAL_WEIGHT) * 100.0f;
      if (WATER_LOSS < 0) WATER_LOSS = 0;
    }

    // 3. Auto-Stop Logic
    if (CURRENT_HUMIDITY <= 14.0 && CURRENT_HUMIDITY > 0) {
      humidityStopCounter++;
      if (humidityStopCounter >= HUMIDITY_STOP_THRESHOLD) {
        stopDrying();
      }
    } else {
      humidityStopCounter = 0;
    }

    // 4. EDT Estimation
    unsigned long now = millis();
    if (now - lastEDTWindowStart >= EDT_WINDOW_MS) {
      float deltaTime = (now - lastEDTWindowStart) / 1000.0f;
      float deltaWeight = weightAtWindowStart - CURRENT_WEIGHT;
      currentDryingRate = deltaWeight / deltaTime; // kg/s

      float targetWeight = INITIAL_WEIGHT * (1.0f - (WATER_LOSS_TARGET / 100.0f));
      float weightToLose = CURRENT_WEIGHT - targetWeight;

      if (currentDryingRate > 0) {
        estimatedEDTSeconds = (long)(weightToLose / currentDryingRate);
      } else {
        estimatedEDTSeconds = -1; // Indeterminate
      }

      lastEDTWindowStart = now;
      weightAtWindowStart = CURRENT_WEIGHT;

      Serial.print("[EDT] Rate: "); Serial.print(currentDryingRate, 6);
      Serial.print(" kg/s, Est EDT: "); Serial.print(estimatedEDTSeconds); Serial.println(" s");
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(SSR1_PIN, OUTPUT);
  pinMode(SSR2_PIN, OUTPUT);
  pinMode(SSR3_PIN, OUTPUT);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  initSHT31();
  initLoadCell();
  initButtons();
  initPID();

  Serial.println("[SYSTEM] FishDryer v2 initialized.");
}

void loop() {
  unsigned long now = millis();

  // Non-blocking sensor and process updates
  if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    updateDryingProcess();
    lastSensorUpdate = now;
  }

  // PID thermostat control
  if (PID_ENABLED && (now - lastPIDUpdate >= PID_UPDATE_INTERVAL)) {
    pidCOMPUTE();
    lastPIDUpdate = now;
  }

  updateButtons();

  // Serial command handling
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "STATUS?") {
      sendStatusJSON();
    } else if (cmd == "PID_START") {
      startDrying();
    } else if (cmd == "PID_STOP") {
      systemState = IDLE;
      PID_ENABLED = false;
      operateSSR(1, false);
      operateSSR(2, false);
      operateSSR(3, false);
      Serial.println("[SYSTEM] Drying stopped manually.");
    } else if (cmd == "TARE") {
      tareLoadCell();
    } else if (cmd.startsWith("SETPOINT:")) {
      TEMPERATURE_SETPOINT = cmd.substring(9).toFloat();
      Serial.print("[CONFIG] New setpoint: "); Serial.println(TEMPERATURE_SETPOINT);
    } else if (cmd.startsWith("WATER_LOSS:")) {
      WATER_LOSS_TARGET = cmd.substring(11).toFloat();
      Serial.print("[CONFIG] New target loss: "); Serial.println(WATER_LOSS_TARGET);
    } else if (cmd == "SHT31:READ") {
      readSHT31();
    } else if (cmd == "LOADCELL:READ") {
      readLoadCell();
    } else if (cmd == "SSR1:1") { operateSSR(1, true); }
    else if (cmd == "SSR1:0") { operateSSR(1, false); }
    else if (cmd == "SSR2:1") { operateSSR(2, true); }
    else if (cmd == "SSR2:0") { operateSSR(2, false); }
    else if (cmd == "SSR3:1") { operateSSR(3, true); }
    else if (cmd == "SSR3:0") { operateSSR(3, false); }
    else {
      Serial.println("Unknown command.");
    }
  }
}
