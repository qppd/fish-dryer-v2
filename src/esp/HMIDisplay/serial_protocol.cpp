// serial_protocol.cpp
// Fish Dryer V2 HMI — ESP-Now communication with NodeMCU Bridge
//
// Replaces UART transport. All public functions keep the same signatures so
// callers (control_screen, diagnostics_screen, etc.) need no changes.

#include "serial_protocol.h"
#include "espnow_protocol.h"
#include "lvgl_v8_port.h"  // for lvgl_port_lock / lvgl_port_unlock
#include <WiFi.h>
#include <esp_now.h>

// ── NodeMCU peer MAC — UPDATE with MAC printed by NodeMCUBridge on boot ───────
// How to find: boot NodeMCUBridge, read "[NodeMCU Bridge] MAC: XX:XX:XX:XX:XX:XX"
// Fill in the 6 bytes below, then re-flash HMIDisplay.
static uint8_t NODEMCU_PEER_MAC[6] = { 0xE0, 0x98, 0x06, 0x8F, 0x98, 0xB9 };  // NodeMCU Bridge MAC

// ── Receive buffer (set in ESP-Now callback, consumed in serialProtoUpdate) ───
static volatile bool        newStatusReceived = false;
static EspNowStatusPacket   pendingStatus;

// ── Connection tracking ───────────────────────────────────────────────────────
#define STATUS_TIMEOUT_MS  6000   // mark disconnected after 6 s with no packet

// ────────────────────────────────────────────────────────────────────
// Internal: map EspNowStatusPacket → DryerData
// (must be called while holding the LVGL mutex)
// ────────────────────────────────────────────────────────────────────
static void applyStatusPacket(const EspNowStatusPacket& pkt) {
    switch (pkt.state) {
        case DSTATE_DRYING:   dryerData.systemState = STATE_DRYING;   break;
        case DSTATE_COMPLETE: dryerData.systemState = STATE_COMPLETE; break;
        case DSTATE_PAUSED:   dryerData.systemState = STATE_PAUSED;   break;
        default:              dryerData.systemState = STATE_IDLE;     break;
    }

    dryerData.heaterOn   = (pkt.flags & FLAG_HEATER)  != 0;
    dryerData.fanOn      = (pkt.flags & FLAG_FAN)     != 0;
    dryerData.exhaustOn  = (pkt.flags & FLAG_EXHAUST) != 0;
    dryerData.pidEnabled = (pkt.flags & FLAG_PID)     != 0;
    dryerData.sht31Detected = (pkt.flags & FLAG_SHT31) != 0;

    dryerData.temperature     = pkt.temperature;
    dryerData.humidity        = pkt.humidity;
    dryerData.weight          = pkt.weight;
    dryerData.waterLoss       = pkt.waterLoss;
    dryerData.targetTemp      = pkt.setpoint;
    dryerData.targetWaterLoss = pkt.waterLossTarget;
    dryerData.pidOutput       = pkt.pidOutput;
    dryerData.dryingElapsedMs = (unsigned long)pkt.runtimeSeconds * 1000UL;

    dryerData.lastUpdateMs = millis();
    dryerData.connected    = true;
}

// ────────────────────────────────────────────────────────────────────
// Internal: send a command packet to NodeMCU Bridge
// ────────────────────────────────────────────────────────────────────
static void sendCmd(uint8_t cmdType, float value = 0.0f) {
    EspNowCmdPacket pkt;
    pkt.packetType = ESPNOW_PKT_CMD;
    pkt.cmdType    = cmdType;
    pkt.value      = value;
    pkt.checksum   = espnowCmdChecksum(&pkt);
    esp_now_send(NODEMCU_PEER_MAC, (const uint8_t*)&pkt, sizeof(pkt));
}

// ── ESP-Now receive callback (runs in Wi-Fi/ISR context — keep it short) ──────
static void onEspNowReceive(const uint8_t* mac,
                             const uint8_t* data, int len) {
    if (len < (int)sizeof(EspNowStatusPacket)) return;
    const EspNowStatusPacket* pkt = (const EspNowStatusPacket*)data;
    if (!espnowStatusValid(pkt)) {
        Serial.println("[ESP-Now] Bad status packet (type/checksum)");
        return;
    }
    if (!newStatusReceived) {
        memcpy((void*)&pendingStatus, pkt, sizeof(EspNowStatusPacket));
        newStatusReceived = true;
    }
}

// ── ESP-Now send callback ─────────────────────────────────────────────────────
static void onEspNowSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ESP-Now] Send failed — check NODEMCU_PEER_MAC");
    }
}

// ────────────────────────────────────────────────────────────────────
void serialProtoInit() {
    // Wi-Fi must be in STA mode (no connection) for ESP-Now
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Print own MAC so user can copy it into NodeMCUBridge HMI_PEER_MAC
    Serial.printf("[HMI] NodeMCU peer MAC (copy to NodeMCUBridge HMI_PEER_MAC): %s\n",
                  WiFi.macAddress().c_str());

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-Now] Init FAILED");
        return;
    }

    esp_now_register_recv_cb((esp_now_recv_cb_t)onEspNowReceive);
    esp_now_register_send_cb(onEspNowSent);

    // Register NodeMCU as peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, NODEMCU_PEER_MAC, 6);
    peerInfo.channel = ESPNOW_WIFI_CHANNEL;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ESP-Now] Add peer failed");
    }

    Serial.printf("[HMI] ESP-Now ready. NodeMCU peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  NODEMCU_PEER_MAC[0], NODEMCU_PEER_MAC[1], NODEMCU_PEER_MAC[2],
                  NODEMCU_PEER_MAC[3], NODEMCU_PEER_MAC[4], NODEMCU_PEER_MAC[5]);
}

void serialProtoUpdate() {
    // Apply any received status packet — acquire LVGL mutex to protect dryerData
    if (newStatusReceived) {
        newStatusReceived = false;
        EspNowStatusPacket pkt;
        memcpy(&pkt, (const void*)&pendingStatus, sizeof(pkt));
        if (lvgl_port_lock(-1)) {
            applyStatusPacket(pkt);
            lvgl_port_unlock();
        }
    }

    // Check connection timeout
    unsigned long now = millis();
    if (dryerData.connected && (now - dryerData.lastUpdateMs > STATUS_TIMEOUT_MS)) {
        if (lvgl_port_lock(-1)) {
            dryerData.connected = false;
            lvgl_port_unlock();
        }
    }
}

// ────────────────────────────────────────────────────────────────────
// Command senders
// ────────────────────────────────────────────────────────────────────
void sendSetTemperature(float temp) {
    sendCmd(CMD_SET_TEMPERATURE, temp);
}

void sendHeaterControl(bool on) {
    sendCmd(on ? CMD_HEATER_ON : CMD_HEATER_OFF);
}

void sendFanControl(bool on) {
    sendCmd(on ? CMD_FAN_ON : CMD_FAN_OFF);
}

void sendExhaustControl(bool on) {
    sendCmd(on ? CMD_EXHAUST_ON : CMD_EXHAUST_OFF);
}

void sendPIDStart() {
    sendCmd(CMD_PID_START);
}

void sendPIDStop() {
    sendCmd(CMD_PID_STOP);
}

void sendStartDrying() {
    dryerData.dryingStartMs = millis();
    dryerData.initialWeight = dryerData.weight;
    sendCmd(CMD_START_DRYING);
}

void sendStopDrying() {
    sendCmd(CMD_STOP_DRYING);
}

void sendStatusRequest() {
    sendCmd(CMD_STATUS_REQUEST);
}

void sendSetWaterLoss(float pct) {
    sendCmd(CMD_SET_WATER_LOSS, pct);
}

void sendTareScale() {
    sendCmd(CMD_TARE_SCALE);
}

void sendCalibrateScale(float knownKg) {
    sendCmd(CMD_CALIBRATE_SCALE, knownKg);
}

void sendSensorTest() {
    sendCmd(CMD_SENSOR_TEST);
}
