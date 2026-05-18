# NodeMCU Bridge Firmware

**File:** `src/esp/NodeMCUBridge/NodeMCUBridge.ino`  
**Target:** NodeMCU 1.0 (ESP-12E / ESP8266)  
**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Overview

The NodeMCU Bridge acts as a **protocol translator** between the Arduino Nano (UART, text protocol) and the HMIDisplay (ESP-Now, binary protocol). It runs a periodic poll loop, caches the latest Nano status, and forwards HMI commands to the Nano.

```
  Nano (UART)  ◄──SoftwareSerial 9600──►  NodeMCU  ◄──ESP-Now (Ch 1)──►  HMI (ESP32-S3)
                                            │
                                      USB Serial (115200) — Debug console
```

## 2. Communication Architecture

| Direction | Protocol | Medium | Speed | Payload |
|-----------|----------|--------|-------|---------|
| Nano → NodeMCU | Text (JSON) | SoftwareSerial D6(RX)/D5(TX) | 9600 baud | `{"temp":58.2,...}` |
| NodeMCU → Nano | Text (commands) | SoftwareSerial D6(TX)/D5(RX) | 9600 baud | `STATUS?`, `PID_START`, etc. |
| HMI → NodeMCU | Binary (ESP-Now) | Wi-Fi (Ch 1) | ~2 Mbps | `EspNowCmdPacket` (7B) |
| NodeMCU → HMI | Binary (ESP-Now) | Wi-Fi (Ch 1) | ~2 Mbps | `EspNowStatusPacket` (38B) |

## 3. Timing

| Interval | Value | Purpose |
|----------|-------|---------|
| `POLL_INTERVAL_MS` | 2000 ms | How often to send `STATUS?` to Nano |
| `ESPNOW_SEND_INTERVAL` | 2000 ms | How often to send status to HMI |
| `NANO_REPLY_TIMEOUT_MS` | 500 ms | Max wait for Nano JSON reply |

## 4. Main Loop

```
Every iteration:

1. Read Nano UART — accumulate characters into nanoRxBuf
   - On '\n': trim, if starts with '{' → store as latestNanoLine
   
2. If new Nano JSON available → parseNanoJSON() → update cachedStatus
   - Extracts: state, flags, temperature, humidity, weight, waterLoss,
               setpoint, waterLossTarget, pidOutput, runtimeSeconds

3. Every 2000ms: send "STATUS?" to Nano (triggers JSON response)

4. If ESP-Now command received (from HMI):
   - forwardCmdToNano() → translate binary cmd to text → send to Nano
   - Immediately request fresh status from Nano (readStringUntil with 500ms timeout)

5. Every 2000ms: send cachedStatus to HMI via esp_now_send()
```

## 5. ESP-Now Callbacks

### `onEspNowReceive()` — Incoming from HMI
- Validates packet length ≥ sizeof(EspNowCmdPacket)
- Validates checksum via `espnowCmdValid()`
- Stores in `pendingCmd` (interrupt-safe, volatile flag)

### `onEspNowSent()` — Outgoing delivery status
- Status 0 = success
- Non-zero = send failed (HMI_PEER_MAC likely incorrect)

## 6. JSON Parsing Helpers

| Function | Purpose |
|----------|---------|
| `jsonFloat(json, key)` | Extract float by key |
| `jsonInt(json, key)` | Extract integer by key |
| `jsonString(json, key)` | Extract quoted string by key |
| `parseNanoJSON(json)` | Full parse → EspNowStatusPacket |

## 7. Command Forwarding

| ESP-Now Cmd | Text Command to Nano |
|-------------|---------------------|
| `CMD_SET_TEMPERATURE` | `SETPOINT:%.2f` |
| `CMD_SET_WATER_LOSS` | `WATER_LOSS:%.2f` |
| `CMD_START_DRYING` / `CMD_PID_START` | `PID_START` |
| `CMD_STOP_DRYING` / `CMD_PID_STOP` | `PID_STOP` |
| `CMD_PAUSE_DRYING` | `PAUSE` |
| `CMD_RESUME_DRYING` | `RESUME` |
| `CMD_HEATER_ON/OFF` | `SSR1:1` / `SSR1:0` |
| `CMD_FAN_ON/OFF` | `SSR2:1` / `SSR2:0` |
| `CMD_EXHAUST_ON/OFF` | `SSR3:1` / `SSR3:0` |
| `CMD_TARE_SCALE` | `TARE` |
| `CMD_CALIBRATE_SCALE` | `CALIBRATE:%.4f` |
| `CMD_SENSOR_TEST` / `CMD_STATUS_REQUEST` | `STATUS?` |

## 8. Configuration: MAC Pairing

The Bridge must know the HMI's MAC address:

```cpp
static uint8_t HMI_PEER_MAC[6] = { 0x80, 0xB5, 0x4E, 0xD3, 0xA5, 0x9C };  // UPDATE ME
```

**Pairing procedure:**
1. Flash NodeMCUBridge first, read its printed MAC
2. Set `NODEMCU_PEER_MAC` in `HMIDisplay/serial_protocol.cpp`
3. Flash HMIDisplay, read its printed MAC
4. Set `HMI_PEER_MAC` in `NodeMCUBridge.ino`
5. Re-flash both (ensure same `ESPNOW_WIFI_CHANNEL`)

## 9. Hardware Wiring

| NodeMCU Pin | GPIO | Signal | Direction | Voltage |
|-------------|------|--------|-----------|---------|
| D5 | GPIO14 | SS_RX | ← from Nano D10 TX | ⚠ 5V → 3.3V (use divider) |
| D6 | GPIO12 | SS_TX | → to Nano D9 RX | 3.3V → 5V (safe) |

**Voltage divider schematic included in source comment.**

## 10. Files

| File | Lines | Purpose |
|------|-------|---------|
| `NodeMCUBridge.ino` | 311 | Full bridge firmware |
| `espnow_protocol.h` | 128 | Shared binary packet definitions |
