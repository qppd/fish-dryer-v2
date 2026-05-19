# ESP-Now Binary Protocol

**File:** `src/esp/HMIDisplay/espnow_protocol.h` (shared)  
**Also in:** `src/esp/NodeMCUBridge/espnow_protocol.h` (identical copy)  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The ESP-Now protocol uses **packed binary structures** for efficient communication between the NodeMCU bridge (ESP8266) and HMIDisplay (ESP32-S3). Both platforms use little-endian Xtensa architecture, making the binary structs directly compatible.

## 2. Common Parameters

| Parameter | Value |
|-----------|-------|
| Wi-Fi Channel | 1 (`ESPNOW_WIFI_CHANNEL`) |
| Peer Role | COMBO (send + receive) |
| Encryption | None |
| Max Packet Size | 250 bytes (ESP-Now limit) |

## 3. Status Packet (NodeMCU → HMI)

**Type ID:** `0x01` (`ESPNOW_PKT_STATUS`)  
**Size:** 38 bytes

```cpp
#pragma pack(push, 1)
typedef struct {
    uint8_t  packetType;      // 0x01 (ESPNOW_PKT_STATUS)
    uint8_t  state;           // DSTATE_IDLE=0, DRYING=1, COMPLETE=2, PAUSED=3
    uint8_t  flags;           // Bitmask: FLAG_HEATER(0x01), FLAG_FAN(0x02), 
                              //         FLAG_EXHAUST(0x04), FLAG_PID(0x08), FLAG_SHT31(0x10)
    float    temperature;     // °C
    float    humidity;        // %RH
    float    weight;          // kg
    float    waterLoss;       // % water lost
    float    setpoint;        // target temperature °C
    float    waterLossTarget; // target water loss %
    float    pidOutput;       // PID output value
    uint16_t runtimeSeconds;  // seconds since drying started
    uint32_t estimatedEDT;    // estimated time to completion (seconds)
    uint8_t  checksum;        // XOR of bytes 0..36
} EspNowStatusPacket;         // total: 38 bytes
#pragma pack(pop)
```

**Field layout:**
| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | packetType |
| 1 | 1 | state |
| 2 | 1 | flags |
| 3 | 4 | temperature |
| 7 | 4 | humidity |
| 11 | 4 | weight |
| 15 | 4 | waterLoss |
| 19 | 4 | setpoint |
| 23 | 4 | waterLossTarget |
| 27 | 4 | pidOutput |
| 31 | 2 | runtimeSeconds |
| 33 | 4 | estimatedEDT |
| 37 | 1 | checksum |

## 4. Command Packet (HMI → NodeMCU)

**Type ID:** `0x02` (`ESPNOW_PKT_CMD`)  
**Size:** 7 bytes

```cpp
#pragma pack(push, 1)
typedef struct {
    uint8_t  packetType;   // 0x02 (ESPNOW_PKT_CMD)
    uint8_t  cmdType;      // CMD_* constant
    float    value;        // optional parameter (0.0 if unused)
    uint8_t  checksum;     // XOR of bytes 0..5
} EspNowCmdPacket;         // total: 7 bytes
#pragma pack(pop)
```

## 5. Command Types

| Command | Code | Value | Description |
|---------|------|-------|-------------|
| `CMD_SET_TEMPERATURE` | 0x01 | target °C | Set temperature setpoint |
| `CMD_START_DRYING` | 0x02 | — | Start drying cycle |
| `CMD_STOP_DRYING` | 0x03 | — | Stop drying cycle |
| `CMD_PAUSE_DRYING` | 0x04 | — | Pause drying |
| `CMD_RESUME_DRYING` | 0x05 | — | Resume drying |
| `CMD_HEATER_ON` | 0x10 | — | Turn heater ON (manual mode) |
| `CMD_HEATER_OFF` | 0x11 | — | Turn heater OFF |
| `CMD_FAN_ON` | 0x12 | — | Turn convection fan ON |
| `CMD_FAN_OFF` | 0x13 | — | Turn convection fan OFF |
| `CMD_EXHAUST_ON` | 0x14 | — | Turn exhaust fan ON |
| `CMD_EXHAUST_OFF` | 0x15 | — | Turn exhaust fan OFF |
| `CMD_PID_START` | 0x20 | — | Alias for CMD_START_DRYING |
| `CMD_PID_STOP` | 0x21 | — | Alias for CMD_STOP_DRYING |
| `CMD_SET_WATER_LOSS` | 0x22 | target % | Set water loss target |
| `CMD_STATUS_REQUEST` | 0x30 | — | Request immediate status |
| `CMD_TARE_SCALE` | 0x40 | — | Tare load cell |
| `CMD_CALIBRATE_SCALE` | 0x41 | known kg | Calibrate with known weight |
| `CMD_SENSOR_TEST` | 0x50 | — | Trigger sensor read on Nano |

## 6. State Values

| Constant | Value | Meaning |
|----------|-------|---------|
| `DSTATE_IDLE` | 0 | System idle |
| `DSTATE_DRYING` | 1 | Drying in progress |
| `DSTATE_COMPLETE` | 2 | Drying complete |
| `DSTATE_PAUSED` | 3 | Drying paused |

## 7. Flags Bitmask

| Flag | Mask | Meaning |
|------|------|---------|
| `FLAG_HEATER` | 0x01 | SSR1 (heating element) is ON |
| `FLAG_FAN` | 0x02 | SSR2 (convection fan) is ON |
| `FLAG_EXHAUST` | 0x04 | SSR3 (exhaust fan) is ON |
| `FLAG_PID` | 0x08 | PID control is enabled |
| `FLAG_SHT31` | 0x10 | SHT31 sensor is detected/operational |

## 8. Checksum Calculation

**Algorithm:** XOR of all bytes except the last (which IS the checksum).

```cpp
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
```

**Validation functions:**
```cpp
static inline bool espnowStatusValid(const EspNowStatusPacket* p) {
    return p->packetType == ESPNOW_PKT_STATUS &&
           p->checksum   == espnowStatusChecksum(p);
}

static inline bool espnowCmdValid(const EspNowCmdPacket* p) {
    return p->packetType == ESPNOW_PKT_CMD &&
           p->checksum   == espnowCmdChecksum(p);
}
```

## 9. MAC Pairing

| Device | MAC Variable Location |
|--------|----------------------|
| NodeMCU MAC → used by HMI | `NODEMCU_PEER_MAC` in `serial_protocol.cpp` |
| HMIDisplay MAC → used by NodeMCU | `HMI_PEER_MAC` in `NodeMCUBridge.ino` |

**Procedure:**
1. Flash NodeMCU → read printed MAC from Serial
2. Update `NODEMCU_PEER_MAC` in HMI code → re-flash HMI
3. Flash HMI → read printed MAC from Serial
4. Update `HMI_PEER_MAC` in NodeMCU code → re-flash NodeMCU
5. Both must use same channel (`ESPNOW_WIFI_CHANNEL`, default 1)

## 10. Protocol Mapping

```
HMI Application Layer
  └─ sendPIDStart(), sendSetTemperature(), etc.
       └─ sendCmd(cmdType, value)
            └─ EspNowCmdPacket → esp_now_send()

NodeMCU Receive
  └─ onEspNowReceive() → store pendingCmd
       └─ forwardCmdToNano() → translate to text cmd
            └─ nano.println("PID_START")

NodeMCU → HMI (reverse path)
  nano.sendStatus() → JSON → parseNanoJSON()
       └─ EspNowStatusPacket → esp_now_send()

HMI Receive
  └─ onEspNowReceive() → store pendingStatus
       └─ serialProtoUpdate() → applyStatusPacket()
            └─ DryerData updated → UI refreshed
```

## 11. Error Handling

- **Bad checksum:** Packet silently dropped (Serial warning printed)
- **Overflow (unread packet):** Previous unread packet is dropped when new one arrives (latest-wins)
- **Send failure:** Non-zero status in send callback; user prompted to check MAC
- **Connection timeout:** No packet for 6s → `dryerData.connected = false`
