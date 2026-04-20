// I2C_COMM.h
// Fish Dryer V2 - I2C Communication Protocol (Nano slave ↔ HMI master)
//
// Arduino Nano = I2C slave  (addr 0x08, hardware I2C on A4/A5)
// HMI ESP32-S3 = I2C master (Wire1 on GPIO 19/20)
//
// HMI reads status  : Wire1.requestFrom(0x08, sizeof(NanoStatusPacket))
// HMI sends commands: Wire1.beginTransmission / write / endTransmission

#ifndef I2C_COMM_H
#define I2C_COMM_H

#include <Arduino.h>
#include <Wire.h>

// =============================================
// I2C Address (same as NanoI2cSample)
// =============================================
#define I2C_SLAVE_ADDR  0x08

// =============================================
// Command Types  (HMI → Nano)
// =============================================
#define CMD_SET_TEMPERATURE   0x01  // value = target °C (float)
#define CMD_START_DRYING      0x02  // no value
#define CMD_STOP_DRYING       0x03  // no value
#define CMD_PAUSE_DRYING      0x04  // no value
#define CMD_HEATER_ON         0x10  // no value
#define CMD_HEATER_OFF        0x11
#define CMD_FAN_ON            0x12
#define CMD_FAN_OFF           0x13
#define CMD_EXHAUST_ON        0x14
#define CMD_EXHAUST_OFF       0x15
#define CMD_PID_START         0x20  // no value (same effect as START_DRYING)
#define CMD_PID_STOP          0x21
#define CMD_SET_WATER_LOSS    0x22  // value = target % (float)
#define CMD_STATUS_REQUEST    0x30  // no value (forces packet rebuild)
#define CMD_TARE_SCALE        0x40  // no value
#define CMD_CALIBRATE_SCALE   0x41  // value = known weight kg (float)
#define CMD_SCAN_I2C          0x50  // no value (retry SHT31 detection)

// =============================================
// NanoStatusPacket  (Nano → HMI, 30 bytes, fits in 32-byte Wire buffer)
// =============================================
#pragma pack(push, 1)
struct NanoStatusPacket {
    uint8_t  state;           // DryingState: 0=IDLE 1=DRYING 2=COMPLETE 3=PAUSED
    uint8_t  flags;           // bit0=heater, bit1=fan, bit2=exhaust, bit3=pid, bit4=sht31
    uint8_t  sensorStatus;    // bits[5:4]=tempStatus, bits[3:2]=humStatus, bits[1:0]=loadStatus
                              // 0=OK 1=WARNING 2=ERROR 3=NOT_FOUND
    float    temperature;     // °C
    float    humidity;        // %
    float    weight;          // kg
    float    waterLoss;       // % water lost
    float    setpoint;        // target temperature °C
    uint16_t waterLossTarget; // target water loss % × 10
    int16_t  pidOutput;       // PID output value (cast from double)
    uint16_t runtimeSeconds;  // seconds since drying started (0 if not drying)
    uint8_t  checksum;        // XOR of bytes 0 to (sizeof-2)
};  // 1+1+1+4+4+4+4+4+2+2+2+1 = 30 bytes
#pragma pack(pop)

// =============================================
// NanoCommandPacket  (HMI → Nano, 6 bytes)
// =============================================
#pragma pack(push, 1)
struct NanoCommandPacket {
    uint8_t cmdType;   // CMD_* constant
    float   value;     // optional value (0 if unused)
    uint8_t checksum;  // XOR of bytes 0 to 4
};  // 6 bytes
#pragma pack(pop)

// =============================================
// Checksum helpers (inline so both sides share)
// =============================================
inline uint8_t i2cCalcChecksum(const uint8_t* data, uint8_t len) {
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++) cs ^= data[i];
    return cs;
}

inline bool i2cVerifyChecksum(const uint8_t* data, uint8_t len) {
    if (len < 2) return false;
    uint8_t cs = 0;
    for (uint8_t i = 0; i < (uint8_t)(len - 1); i++) cs ^= data[i];
    return cs == data[len - 1];
}

#endif // I2C_COMM_H
