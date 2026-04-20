// serial_protocol.h
// Fish Dryer V2 HMI — ESP-Now communication with NodeMCU Bridge
//
// Public API is unchanged — all callers (control_screen, diagnostics_screen, etc.)
// continue to call the same functions; only the transport has changed from UART to
// ESP-Now.
//
// ── MAC Setup ──────────────────────────────────────────────────────────────────
// 1. Boot NodeMCUBridge, read "MAC: XX:XX:XX:XX:XX:XX" on its Serial.
// 2. Paste that MAC into NODEMCU_PEER_MAC in serial_protocol.cpp, re-flash HMI.
// 3. Boot HMIDisplay, read "NodeMCU peer MAC: XX:XX:XX:XX:XX:XX" on its Serial.
// 4. Paste that MAC into HMI_PEER_MAC in NodeMCUBridge.ino, re-flash NodeMCU.

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>
#include "dryer_data.h"

// Initialize ESP-Now communication
void serialProtoInit();

// Call from loop() — receives status packets, checks connection timeout
void serialProtoUpdate();

// Commands to send to NodeMCU Bridge (forwarded to Nano over UART)
void sendSetTemperature(float temp);
void sendHeaterControl(bool on);
void sendFanControl(bool on);
void sendExhaustControl(bool on);
void sendPIDStart();
void sendPIDStop();
void sendStartDrying();
void sendStopDrying();
void sendStatusRequest();
void sendSetWaterLoss(float pct);
void sendTareScale();
void sendCalibrateScale(float knownKg);
void sendSensorTest();   // triggers SHT31 + load-cell read on Nano

#endif // SERIAL_PROTOCOL_H
