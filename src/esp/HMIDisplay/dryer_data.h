// dryer_data.h
// Shared data structures for Fish Dryer V2 HMI

#ifndef DRYER_DATA_H
#define DRYER_DATA_H

#include <Arduino.h>

// System states
enum DryerState {
    STATE_IDLE,
    STATE_DRYING,
    STATE_COMPLETE,
    STATE_ERROR,
    STATE_PAUSED
};

// Sensor status for diagnostics
enum SensorStatus {
    SENSOR_OK,
    SENSOR_WARNING,
    SENSOR_ERROR,
    SENSOR_NOT_FOUND
};

// Shared data structure - updated by serial protocol, read by screens
struct DryerData {
    // Sensor readings
    float temperature;
    float humidity;
    float weight;
    float waterLoss;

    // Setpoints
    float targetTemp;
    float targetWaterLoss;

    // Relay states
    bool heaterOn;
    bool fanOn;
    bool exhaustOn;

    // Control state
    bool pidEnabled;
    DryerState systemState;
    bool dryingModeAuto;  // true = Auto, false = Manual

    // PID info
    float pidOutput;

    // Diagnostics
    SensorStatus tempSensorStatus;
    SensorStatus loadCellStatus;
    float fanCurrent;
    float heaterPower;
    float batteryVoltage;
    bool sht31Detected;

    // Connection
    unsigned long lastUpdateMs;
    bool connected;

    // Drying session
    unsigned long dryingStartMs;
    unsigned long dryingElapsedMs;
    float initialWeight;
};

// Global dryer data instance
extern DryerData dryerData;

// Initialize dryer data with defaults
inline void initDryerData() {
    dryerData.temperature = 0.0f;
    dryerData.humidity = 0.0f;
    dryerData.weight = 0.0f;
    dryerData.waterLoss = 0.0f;
    dryerData.targetTemp = 60.0f;
    dryerData.targetWaterLoss = 70.0f;
    dryerData.heaterOn = false;
    dryerData.fanOn = false;
    dryerData.exhaustOn = false;
    dryerData.pidEnabled = false;
    dryerData.systemState = STATE_IDLE;
    dryerData.dryingModeAuto = true;
    dryerData.pidOutput = 0.0f;
    dryerData.tempSensorStatus = SENSOR_OK;
    dryerData.loadCellStatus = SENSOR_OK;
    dryerData.fanCurrent = 0.0f;
    dryerData.heaterPower = 0.0f;
    dryerData.batteryVoltage = 0.0f;
    dryerData.sht31Detected = false;
    dryerData.lastUpdateMs = 0;
    dryerData.connected = false;
    dryerData.dryingStartMs = 0;
    dryerData.dryingElapsedMs = 0;
    dryerData.initialWeight = 0.0f;
}

#endif // DRYER_DATA_H
