// espnow_protocol.h
// Fish Dryer V2 — Shared ESP-Now binary protocol
//
// NodeMCU (ESP8266) <-- ESP-Now --> HMIDisplay (ESP32-S3)
//
// Packet types:
//   0x01  EspNowStatusPacket  — NodeMCU → HMI  (Nano sensor/state data)
//   0x02  EspNowCmdPacket     — HMI → NodeMCU  (control commands)
//
// Both ESP8266 and ESP32 are little-endian Xtensa.
// Packed structs are binary-compatible between the two.
//
// ── MAC Setup ──────────────────────────────────────────────────────────────────
// Each device prints its own MAC on Serial at boot. Update the peer MAC in the
// OTHER device's firmware to match.
//
//   NodeMCUBridge.ino  : #define HMI_PEER_MAC  {0xXX,…}   ← paste HMI MAC here
//   serial_protocol.cpp: #define NODEMCU_PEER_MAC {0xXX,…} ← paste NodeMCU MAC here
//
// ── ESP-Now channel ────────────────────────────────────────────────────────────
// Both devices must use the same Wi-Fi channel (default 1).
// Change ESPNOW_WIFI_CHANNEL if your network uses a different channel.

#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <stdint.h>

// ── Wi-Fi channel ──────────────────────────────────────────────────────────────
#define ESPNOW_WIFI_CHANNEL  1

// ── Packet type IDs ───────────────────────────────────────────────────────────
#define ESPNOW_PKT_STATUS    0x01
#define ESPNOW_PKT_CMD       0x02

// ── Command type IDs (HMI → NodeMCU → Nano) ──────────────────────────────────
#define CMD_SET_TEMPERATURE  0x01   // value = target °C
#define CMD_START_DRYING     0x02   // no value
#define CMD_STOP_DRYING      0x03   // no value
#define CMD_PAUSE_DRYING     0x04   // no value
#define CMD_RESUME_DRYING    0x05   // no value
#define CMD_HEATER_ON        0x10   // no value
#define CMD_HEATER_OFF       0x11   // no value
#define CMD_FAN_ON           0x12   // no value
#define CMD_FAN_OFF          0x13   // no value
#define CMD_EXHAUST_ON       0x14   // no value
#define CMD_EXHAUST_OFF      0x15   // no value
#define CMD_PID_START        0x20   // no value (alias START_DRYING)
#define CMD_PID_STOP         0x21   // no value (alias STOP_DRYING)
#define CMD_SET_WATER_LOSS   0x22   // value = target %
#define CMD_STATUS_REQUEST   0x30   // no value (force immediate poll)
#define CMD_TARE_SCALE       0x40   // no value
#define CMD_CALIBRATE_SCALE  0x41   // value = known weight kg
#define CMD_SENSOR_TEST      0x50   // no value (trigger SHT31+LOADCELL read)

// ── Dryer state values ────────────────────────────────────────────────────────
#define DSTATE_IDLE      0
#define DSTATE_DRYING    1
#define DSTATE_COMPLETE  2
#define DSTATE_PAUSED    3

// ── Status flags (EspNowStatusPacket.flags) ───────────────────────────────────
#define FLAG_HEATER  0x01
#define FLAG_FAN     0x02
#define FLAG_EXHAUST 0x04
#define FLAG_PID     0x08
#define FLAG_SHT31   0x10

// ────────────────────────────────────────────────────────────────────
// EspNowStatusPacket — NodeMCU → HMI  (34 bytes, well under 250-byte limit)
// ────────────────────────────────────────────────────────────────────
#pragma pack(push, 1)
typedef struct {
    uint8_t  packetType;      // ESPNOW_PKT_STATUS (0x01)
    uint8_t  state;           // DSTATE_*
    uint8_t  flags;           // FLAG_* bitmask
    float    temperature;     // °C
    float    humidity;        // %
    float    weight;          // kg
    float    waterLoss;       // % water lost so far
    float    setpoint;        // target temperature °C
    float    waterLossTarget; // target water loss %
    float    pidOutput;       // PID output
    uint16_t runtimeSeconds;  // seconds since drying started (0 if not drying)
    uint8_t  checksum;        // XOR of bytes [0 .. sizeof-2]
} EspNowStatusPacket;         // 1+1+1+4+4+4+4+4+4+4+2+1 = 34 bytes
#pragma pack(pop)

// ────────────────────────────────────────────────────────────────────
// EspNowCmdPacket — HMI → NodeMCU  (7 bytes)
// ────────────────────────────────────────────────────────────────────
#pragma pack(push, 1)
typedef struct {
    uint8_t  packetType;   // ESPNOW_PKT_CMD (0x02)
    uint8_t  cmdType;      // CMD_*
    float    value;        // optional float value (0.0 if unused)
    uint8_t  checksum;     // XOR of bytes [0 .. sizeof-2]
} EspNowCmdPacket;         // 1+1+4+1 = 7 bytes
#pragma pack(pop)

// ── Checksum helpers (inline — available on both platforms) ───────────────────
static inline uint8_t espnowCalcChecksum(const uint8_t* data, uint8_t len) {
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++) cs ^= data[i];
    return cs;
}

static inline uint8_t espnowStatusChecksum(const EspNowStatusPacket* p) {
    return espnowCalcChecksum((const uint8_t*)p, sizeof(EspNowStatusPacket) - 1);
}

static inline uint8_t espnowCmdChecksum(const EspNowCmdPacket* p) {
    return espnowCalcChecksum((const uint8_t*)p, sizeof(EspNowCmdPacket) - 1);
}

static inline bool espnowStatusValid(const EspNowStatusPacket* p) {
    return p->packetType == ESPNOW_PKT_STATUS &&
           p->checksum   == espnowStatusChecksum(p);
}

static inline bool espnowCmdValid(const EspNowCmdPacket* p) {
    return p->packetType == ESPNOW_PKT_CMD &&
           p->checksum   == espnowCmdChecksum(p);
}

#endif // ESPNOW_PROTOCOL_H
