// serial_protocol.h
// Fish Dryer V2 HMI - UART communication with dryer controller

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>
#include "dryer_data.h"

// UART configuration for dryer communication
// Adjust these pins for your Waveshare ESP32-S3-Touch-LCD-7 wiring
#define DRYER_UART_RX_PIN   15
#define DRYER_UART_TX_PIN   16
#define DRYER_UART_BAUD     115200
#define DRYER_UART          Serial2

// Status request interval
#define STATUS_REQUEST_MS   2000

// Initialize UART communication
void serialProtoInit();

// Call from loop() - reads incoming data, sends periodic status requests
void serialProtoUpdate();

// Commands to send to dryer controller
void sendSetTemperature(float temp);
void sendHeaterControl(bool on);
void sendFanControl(bool on);
void sendExhaustControl(bool on);
void sendPIDStart();
void sendPIDStop();
void sendStartDrying();
void sendStopDrying();
void sendStatusRequest();

#endif // SERIAL_PROTOCOL_H
