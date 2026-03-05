// serial_protocol.cpp
// Fish Dryer V2 HMI - UART communication implementation

#include "serial_protocol.h"
#include "lvgl_v8_port.h"  // for lvgl_port_lock / lvgl_port_unlock

static String rxBuffer;
static unsigned long lastStatusRequest = 0;

// Simple JSON value parser - extracts a float value for a given key
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

// Simple JSON value parser - extracts an int value for a given key
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

// Simple JSON string parser - extracts a string value for a given key
static String jsonString(const String& json, const char* key) {
    String search = String("\"") + key + "\":\"";
    int idx = json.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = json.indexOf('"', idx);
    if (end < 0) return "";
    return json.substring(idx, end);
}

// Parse incoming JSON message and update DryerData
static void parseJsonMessage(const String& json) {
    // Expected format:
    // {"temp":58.4,"hum":32,"weight":3.2,"loss":45,"heater":1,"fan":1,
    //  "exhaust":0,"pid":1,"state":"DRYING","target_temp":60,"target_loss":70,
    //  "pid_out":2500,"fan_a":0.3,"heater_w":980,"bat_v":12.4,"sht31":1}

    dryerData.temperature = jsonFloat(json, "temp");
    dryerData.humidity = jsonFloat(json, "hum");
    dryerData.weight = jsonFloat(json, "weight");
    dryerData.waterLoss = jsonFloat(json, "loss");

    dryerData.heaterOn = jsonInt(json, "heater") != 0;
    dryerData.fanOn = jsonInt(json, "fan") != 0;
    dryerData.exhaustOn = jsonInt(json, "exhaust") != 0;
    dryerData.pidEnabled = jsonInt(json, "pid") != 0;

    dryerData.targetTemp = jsonFloat(json, "target_temp");
    dryerData.targetWaterLoss = jsonFloat(json, "target_loss");
    dryerData.pidOutput = jsonFloat(json, "pid_out");

    // Diagnostics data
    dryerData.fanCurrent = jsonFloat(json, "fan_a");
    dryerData.heaterPower = jsonFloat(json, "heater_w");
    dryerData.batteryVoltage = jsonFloat(json, "bat_v");
    dryerData.sht31Detected = jsonInt(json, "sht31") != 0;

    // System state
    String state = jsonString(json, "state");
    if (state == "DRYING")        dryerData.systemState = STATE_DRYING;
    else if (state == "COMPLETE") dryerData.systemState = STATE_COMPLETE;
    else if (state == "ERROR")    dryerData.systemState = STATE_ERROR;
    else if (state == "PAUSED")   dryerData.systemState = STATE_PAUSED;
    else                          dryerData.systemState = STATE_IDLE;

    dryerData.lastUpdateMs = millis();
    dryerData.connected = true;
}

// Parse legacy text format: "KEY:VALUE"
static void parseTextMessage(const String& line) {
    int sep = line.indexOf(':');
    if (sep < 0) return;

    String key = line.substring(0, sep);
    String val = line.substring(sep + 1);
    key.trim();
    val.trim();

    if (key == "TEMP" || key == "SHT31 Temp") {
        dryerData.temperature = val.toFloat();
    } else if (key == "HUM" || key == "Humidity") {
        dryerData.humidity = val.toFloat();
    } else if (key == "WEIGHT") {
        dryerData.weight = val.toFloat();
    } else if (key == "LOSS") {
        dryerData.waterLoss = val.toFloat();
    }

    dryerData.lastUpdateMs = millis();
    dryerData.connected = true;
}

// Process a complete line from UART
static void processLine(const String& line) {
    if (line.length() == 0) return;

    if (line.charAt(0) == '{') {
        parseJsonMessage(line);
    } else {
        parseTextMessage(line);
    }
}

void serialProtoInit() {
    DRYER_UART.begin(DRYER_UART_BAUD, SERIAL_8N1, DRYER_UART_RX_PIN, DRYER_UART_TX_PIN);
    rxBuffer.reserve(512);
    Serial.println("[HMI] Serial protocol initialized");
}

void serialProtoUpdate() {
    // Build complete lines; only touch dryerData while holding the LVGL mutex
    // to prevent a data race with the LVGL timer task that reads dryerData.
    while (DRYER_UART.available()) {
        char c = DRYER_UART.read();
        if (c == '\n') {
            rxBuffer.trim();
            if (rxBuffer.length() > 0) {
                if (lvgl_port_lock(-1)) {
                    processLine(rxBuffer);
                    lvgl_port_unlock();
                }
            }
            rxBuffer = "";
        } else if (c != '\r') {
            if (rxBuffer.length() < 500) {
                rxBuffer += c;
            }
        }
    }

    // Periodically request status from dryer
    unsigned long now = millis();
    if (now - lastStatusRequest >= STATUS_REQUEST_MS) {
        sendStatusRequest();
        lastStatusRequest = now;
    }

    // Check connection timeout (5 seconds)
    if (dryerData.connected && (now - dryerData.lastUpdateMs > 5000)) {
        if (lvgl_port_lock(-1)) {
            dryerData.connected = false;
            lvgl_port_unlock();
        }
    }
}

// Command senders - using existing FishDryer text protocol
void sendSetTemperature(float temp) {
    DRYER_UART.print("PID:SET:");
    DRYER_UART.println(temp, 1);
}

void sendHeaterControl(bool on) {
    DRYER_UART.println(on ? "SSR1:1" : "SSR1:0");
}

void sendFanControl(bool on) {
    DRYER_UART.println(on ? "SSR2:1" : "SSR2:0");
}

void sendExhaustControl(bool on) {
    DRYER_UART.println(on ? "SSR3:1" : "SSR3:0");
}

void sendPIDStart() {
    DRYER_UART.println("PID:START");
}

void sendPIDStop() {
    DRYER_UART.println("PID:STOP");
}

void sendStartDrying() {
    dryerData.dryingStartMs = millis();
    dryerData.initialWeight = dryerData.weight;
    sendPIDStart();
}

void sendStopDrying() {
    sendPIDStop();
}

void sendStatusRequest() {
    DRYER_UART.println("STATUS");
}
