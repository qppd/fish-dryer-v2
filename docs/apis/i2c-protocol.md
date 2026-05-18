# I2C Communication Protocol (Legacy / Reference)

**File:** `src/nano/FishDryer/I2C_COMM.h`  
**Current Status:** Reference implementation — NOT in active use  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The I2C protocol was designed for direct communication between the Arduino Nano (I2C slave) and the HMIDisplay (I2C master) on the hardware I2C bus (A4/A5). However, the current implementation uses an **ESP-Now + UART bridge** path instead.

This document is preserved as a reference for future development.

## 2. Bus Configuration

| Parameter | Value |
|-----------|-------|
| Nano Role | I2C Slave (address 0x08) |
| HMI Role | I2C Master (Wire1, GPIO 19/20) |
| Speed | 100 kHz (standard mode) |
| Nano Pins | A4 (SDA), A5 (SCL) |
| Packet Size | ≤ 32 bytes (Wire buffer limit) |

## 3. NanoStatusPacket (Nano → HMI)

**Size:** 30 bytes

```cpp
#pragma pack(push, 1)
struct NanoStatusPacket {
    uint8_t  state;           // 0=IDLE, 1=DRYING, 2=COMPLETE, 3=PAUSED
    uint8_t  flags;           // bit0=heater, bit1=fan, bit2=exhaust, bit3=pid, bit4=sht31
    uint8_t  sensorStatus;    // bits[5:4]=temp, bits[3:2]=hum, bits[1:0]=load
    float    temperature;     // °C
    float    humidity;        // %RH
    float    weight;          // kg
    float    waterLoss;       // %
    float    setpoint;        // target °C
    uint16_t waterLossTarget; // target % × 10
    int16_t  pidOutput;       // cast from double
    uint16_t runtimeSeconds;  // seconds
    uint8_t  checksum;        // XOR of bytes 0..28
};
#pragma pack(pop)
```

## 4. NanoCommandPacket (HMI → Nano)

**Size:** 6 bytes

```cpp
#pragma pack(push, 1)
struct NanoCommandPacket {
    uint8_t cmdType;   // CMD_* constant
    float   value;     // optional parameter
    uint8_t checksum;  // XOR of bytes 0-4
};
#pragma pack(pop)
```

## 5. Command Types (Identical to ESP-Now)

Same command constants as `espnow_protocol.h`:
- `CMD_SET_TEMPERATURE` (0x01) through `CMD_SCAN_I2C` (0x50)

## 6. Why I2C Wasn't Used

| Factor | I2C | Current (UART + ESP-Now) |
|--------|-----|--------------------------|
| Distance | Short (< 1m) | Longer range (Wi-Fi) |
| Wiring | Extra SDA/SCL lines | Shared D9/D10 serial |
| HMI board | Dedicated I2C pins | ESP-Now wireless |
| Complexity | Master/slave synchronization | Poll-based simpler |

## 7. Re-enabling I2C

To switch back to I2C:
1. Enable Wire slave in Nano setup: `Wire.begin(I2C_SLAVE_ADDR)`
2. Set `onRequest` callback for status packets
3. Set `onReceive` callback for command packets
4. Update HMIDisplay to use `Wire1.requestFrom()` instead of ESP-Now
