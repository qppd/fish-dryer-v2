/**
 * NodeMCUBridge.ino — ESP8266 NodeMCU 1.0 — UART↔ESP-Now Bridge
 *
 * Role: bridge between Arduino Nano (FishDryer hardware) and ESP32-S3 HMIDisplay.
 *
 *   Nano <──SoftwareSerial 9600──> NodeMCU <──ESP-Now──> HMIDisplay (ESP32-S3)
 *
 * ── UART (to Nano) ────────────────────────────────────────────────────────────
 *   NodeMCU D6 / GPIO12  (SS_TX) ──wire──> Nano A4 (SS_RX)   3.3 V → 5 V OK
 *   NodeMCU D5 / GPIO14  (SS_RX) ──wire──> Nano A5 (SS_TX)   5 V  → 3.3 V !
 *     Use voltage divider on Nano A5 line:
 *       Nano A5 --[10 kΩ]--+-- NodeMCU D5
 *                           |
 *                        [20 kΩ]
 *                           |
 *                          GND
 *
 * ── ESP-Now (to HMIDisplay) ───────────────────────────────────────────────────
 *   Channel : ESPNOW_WIFI_CHANNEL (default 1)
 *   Peer    : HMI_PEER_MAC — update with the MAC printed by HMIDisplay on boot
 *
 * ── MAC Setup ─────────────────────────────────────────────────────────────────
 *   1. Flash HMIDisplay, open Serial Monitor, copy the printed MAC (e.g. 24:6F:28:AA:BB:CC)
 *   2. Update HMI_PEER_MAC below and re-flash NodeMCUBridge.
 *   3. Flash NodeMCUBridge, open Serial Monitor, copy the printed MAC.
 *   4. Update NODEMCU_PEER_MAC in HMIDisplay/serial_protocol.cpp and re-flash HMI.
 *
 * ── Required libraries ────────────────────────────────────────────────────────
 *   Board: "NodeMCU 1.0 (ESP-12E Module)"  in Arduino IDE
 *   Install "EspSoftwareSerial" (by plerup) from Library Manager.
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#include "espnow_protocol.h"

// ── Peer MAC — UPDATE WITH YOUR HMIDisplay MAC ────────────────────────────────
// How to find: boot HMIDisplay, read "NodeMCU peer MAC: XX:XX:XX:XX:XX:XX" on Serial.
// Fill in the 6 bytes below, then re-flash this sketch.
static uint8_t HMI_PEER_MAC[6] = { 0x80, 0xB5, 0x4E, 0xD3, 0xA5, 0x9C };  // HMIDisplay MAC

// ── SoftwareSerial to Nano (D5=RX, D6=TX) ─────────────────────────────────────
#define SS_TX_PIN  D6    // GPIO12 → Nano A4 (SS RX)
#define SS_RX_PIN  D5    // GPIO14 ← Nano A5 (SS TX)  use voltage divider!
#define UART_BAUD  9600

SoftwareSerial nano(SS_RX_PIN, SS_TX_PIN);

// ── Timing ────────────────────────────────────────────────────────────────────
#define POLL_INTERVAL_MS      2000   // how often to ask Nano for status
#define NANO_REPLY_TIMEOUT_MS  500   // max wait after STATUS?
#define ESPNOW_SEND_INTERVAL  2000   // max ms between ESP-Now status sends

static unsigned long lastPollMs    = 0;
static unsigned long lastSendMs    = 0;

// ── Nano UART receive buffer ──────────────────────────────────────────────────
static String nanoRxBuf;
static bool   newNanoLine = false;
static String latestNanoLine;

// ── ESP-Now receive buffer (set in ISR, consumed in loop) ────────────────────
static volatile bool     newCmdReceived = false;
static EspNowCmdPacket   pendingCmd;

// ── Cached status (updated from Nano JSON, sent to HMI) ──────────────────────
static EspNowStatusPacket cachedStatus;

// ────────────────────────────────────────────────────────────────────
// JSON helpers
// ────────────────────────────────────────────────────────────────────
static float jsonFloat(const String& json, const char* key) {
    String search = String("\"") + key + "\"";
    int idx = json.indexOf(search);
    if (idx < 0) return 0.0f;
    idx = json.indexOf(':', idx);
    if (idx < 0) return 0.0f;
    idx++;
    while (idx < (int)json.length() && json[idx] == ' ') idx++;
    return json.substring(idx).toFloat();
}

static int jsonInt(const String& json, const char* key) {
    String search = String("\"") + key + "\"";
    int idx = json.indexOf(search);
    if (idx < 0) return 0;
    idx = json.indexOf(':', idx);
    if (idx < 0) return 0;
    idx++;
    while (idx < (int)json.length() && json[idx] == ' ') idx++;
    return json.substring(idx).toInt();
}

static String jsonString(const String& json, const char* key) {
    String search = String("\"") + key + "\":\"";
    int idx = json.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = json.indexOf('"', idx);
    if (end < 0) return "";
    return json.substring(idx, end);
}

// ── Parse Nano JSON → EspNowStatusPacket ─────────────────────────────────────
static void parseNanoJSON(const String& json) {
    cachedStatus.packetType     = ESPNOW_PKT_STATUS;

    String stateStr = jsonString(json, "state");
    if      (stateStr == "DRYING")   cachedStatus.state = DSTATE_DRYING;
    else if (stateStr == "COMPLETE") cachedStatus.state = DSTATE_COMPLETE;
    else if (stateStr == "PAUSED")   cachedStatus.state = DSTATE_PAUSED;
    else                             cachedStatus.state = DSTATE_IDLE;

    uint8_t flags = 0;
    if (jsonInt(json, "heater")) flags |= FLAG_HEATER;
    if (jsonInt(json, "fan"))    flags |= FLAG_FAN;
    if (jsonInt(json, "exhaust"))flags |= FLAG_EXHAUST;
    if (jsonInt(json, "pid"))    flags |= FLAG_PID;
    if (jsonInt(json, "sht31"))  flags |= FLAG_SHT31;
    cachedStatus.flags = flags;

    cachedStatus.temperature     = jsonFloat(json, "temp");
    cachedStatus.humidity        = jsonFloat(json, "hum");
    cachedStatus.weight          = jsonFloat(json, "weight");
    cachedStatus.waterLoss       = jsonFloat(json, "loss");
    cachedStatus.setpoint        = jsonFloat(json, "target_temp");
    cachedStatus.waterLossTarget = jsonFloat(json, "target_loss");
    cachedStatus.pidOutput       = jsonFloat(json, "pid_out");
    cachedStatus.runtimeSeconds  = (uint16_t)jsonInt(json, "runtime");

    cachedStatus.checksum = espnowStatusChecksum(&cachedStatus);
}

// ── Send cached status to HMI via ESP-Now ────────────────────────────────────
static void sendStatusToHMI() {
    esp_now_send(HMI_PEER_MAC, (uint8_t*)&cachedStatus, sizeof(EspNowStatusPacket));
    lastSendMs = millis();
}

// ── Translate EspNowCmdPacket → UART text → send to Nano ─────────────────────
static void forwardCmdToNano(const EspNowCmdPacket& cmd) {
    char buf[32];
    switch (cmd.cmdType) {
        case CMD_SET_TEMPERATURE:
            snprintf(buf, sizeof(buf), "SETPOINT:%.2f", cmd.value);
            nano.println(buf);
            break;
        case CMD_SET_WATER_LOSS:
            snprintf(buf, sizeof(buf), "WATER_LOSS:%.2f", cmd.value);
            nano.println(buf);
            break;
        case CMD_START_DRYING:
        case CMD_PID_START:
            nano.println("PID_START");
            break;
        case CMD_STOP_DRYING:
        case CMD_PID_STOP:
            nano.println("PID_STOP");
            break;
        case CMD_PAUSE_DRYING:
            nano.println("PAUSE");
            break;
        case CMD_RESUME_DRYING:
            nano.println("RESUME");
            break;
        case CMD_HEATER_ON:    nano.println("SSR1:1"); break;
        case CMD_HEATER_OFF:   nano.println("SSR1:0"); break;
        case CMD_FAN_ON:       nano.println("SSR2:1"); break;
        case CMD_FAN_OFF:      nano.println("SSR2:0"); break;
        case CMD_EXHAUST_ON:   nano.println("SSR3:1"); break;
        case CMD_EXHAUST_OFF:  nano.println("SSR3:0"); break;
        case CMD_TARE_SCALE:   nano.println("TARE");   break;
        case CMD_CALIBRATE_SCALE:
            snprintf(buf, sizeof(buf), "CALIBRATE:%.4f", cmd.value);
            nano.println(buf);
            break;
        case CMD_SENSOR_TEST:
        case CMD_STATUS_REQUEST:
            nano.println("STATUS?");
            break;
        default:
            Serial.printf("[Bridge] Unknown cmd 0x%02X\n", cmd.cmdType);
            break;
    }
    Serial.printf("[Bridge] → Nano: %s (cmd 0x%02X val=%.2f)\n",
                  buf, cmd.cmdType, cmd.value);
}

// ── ESP-Now receive callback (runs in interrupt context) ─────────────────────
static void onEspNowReceive(uint8_t* mac, uint8_t* data, uint8_t len) {
    if (len < sizeof(EspNowCmdPacket)) return;
    const EspNowCmdPacket* pkt = (const EspNowCmdPacket*)data;
    if (!espnowCmdValid(pkt)) {
        Serial.println("[ESP-Now] Bad checksum on received cmd");
        return;
    }
    if (!newCmdReceived) {  // drop if previous not yet consumed
        memcpy((void*)&pendingCmd, pkt, sizeof(EspNowCmdPacket));
        newCmdReceived = true;
    }
}

// ── ESP-Now send callback ─────────────────────────────────────────────────────
static void onEspNowSent(uint8_t* mac, uint8_t status) {
    // status 0 = success
    if (status != 0) {
        Serial.println("[ESP-Now] Send failed — is HMI_PEER_MAC correct?");
    }
}

// ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[NodeMCU Bridge] Fish Dryer V2 starting...");

    // Print own MAC for user to copy into HMIDisplay serial_protocol.cpp
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.printf("[NodeMCU Bridge] MAC: %s\n", WiFi.macAddress().c_str());
    Serial.println("[NodeMCU Bridge] Copy MAC above → NODEMCU_PEER_MAC in HMIDisplay/serial_protocol.cpp");

    // Init ESP-Now
    if (esp_now_init() != 0) {
        Serial.println("[ESP-Now] Init FAILED — halting");
        while (true) delay(1000);
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb((esp_now_recv_cb_t)onEspNowReceive);
    esp_now_register_send_cb((esp_now_send_cb_t)onEspNowSent);

    // Register HMI as ESP-Now peer
    esp_now_add_peer(HMI_PEER_MAC, ESP_NOW_ROLE_COMBO, ESPNOW_WIFI_CHANNEL, NULL, 0);
    Serial.printf("[ESP-Now] HMI peer: %02X:%02X:%02X:%02X:%02X:%02X  ch=%d\n",
                  HMI_PEER_MAC[0], HMI_PEER_MAC[1], HMI_PEER_MAC[2],
                  HMI_PEER_MAC[3], HMI_PEER_MAC[4], HMI_PEER_MAC[5],
                  ESPNOW_WIFI_CHANNEL);

    // Init SoftwareSerial to Nano
    nanoRxBuf.reserve(256);
    nano.begin(UART_BAUD);
    Serial.printf("[UART] SoftwareSerial ready  TX=D6(GPIO12)  RX=D5(GPIO14)  baud=%d\n",
                  UART_BAUD);

    // Clear cached status
    memset(&cachedStatus, 0, sizeof(cachedStatus));
    cachedStatus.packetType = ESPNOW_PKT_STATUS;

    Serial.println("[NodeMCU Bridge] Ready.");
}

// ────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── 1. Read Nano UART — build lines ──────────────────────────────
    while (nano.available()) {
        char c = (char)nano.read();
        if (c == '\n') {
            nanoRxBuf.trim();
            if (nanoRxBuf.length() > 0 && nanoRxBuf.charAt(0) == '{') {
                latestNanoLine = nanoRxBuf;
                newNanoLine    = true;
            }
            nanoRxBuf = "";
        } else if (c != '\r') {
            if (nanoRxBuf.length() < 240) nanoRxBuf += c;
        }
    }

    // ── 2. Parse latest Nano JSON if available ────────────────────────
    if (newNanoLine) {
        newNanoLine = false;
        parseNanoJSON(latestNanoLine);
        Serial.print("[Nano] Status: "); Serial.println(latestNanoLine);
    }

    // ── 3. Periodic poll: send STATUS? to Nano ────────────────────────
    if (now - lastPollMs >= POLL_INTERVAL_MS) {
        lastPollMs = now;
        nano.println("STATUS?");
    }

    // ── 4. Forward pending ESP-Now command to Nano ────────────────────
    if (newCmdReceived) {
        newCmdReceived = false;
        EspNowCmdPacket cmd;
        memcpy(&cmd, (const void*)&pendingCmd, sizeof(cmd));
        forwardCmdToNano(cmd);
        // Immediately request fresh status after any command
        nano.setTimeout(NANO_REPLY_TIMEOUT_MS);
        String reply = nano.readStringUntil('\n');
        reply.trim();
        if (reply.charAt(0) == '{') {
            parseNanoJSON(reply);
        }
    }

    // ── 5. Periodically send status to HMI via ESP-Now ───────────────
    if (now - lastSendMs >= ESPNOW_SEND_INTERVAL) {
        sendStatusToHMI();
    }

    yield();  // allow ESP8266 background tasks (TCP/IP stack, etc.)
}
