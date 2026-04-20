// FishDryer.ino
// Arduino Nano-based fish dryer controller
// Communication: SoftwareSerial UART to NodeMCU Bridge (D9=RX, D10=TX, 9600 baud)
// Also accepts the same command set from Serial Monitor (115200 baud).
// NodeMCU polls status every 2 s and forwards commands from HMIDisplay via ESP-Now.

#include <SoftwareSerial.h>
#include "PINS_CONFIG.h"
#include "SSR_CONFIG.h"
#include "SHT31_CONFIG.h"
#include "LOADCELL_CONFIG.h"
#include "PID_CONFIG.h"

// PID / temperature globals
double CURRENT_TEMPERATURE = 0;
double TEMPERATURE_SETPOINT = 60.0;
bool PID_ENABLED = false;
unsigned long lastPIDUpdate = 0;

// Drying state
enum DryingState : uint8_t { STATE_IDLE, STATE_DRYING, STATE_COMPLETE, STATE_PAUSED };
DryingState systemState = STATE_IDLE;
float WATER_LOSS_TARGET  = 70.0f;
float INITIAL_WEIGHT     = 0.0f;
float CURRENT_WEIGHT     = 0.0f;
float CURRENT_HUMIDITY   = 0.0f;
float CURRENT_WATER_LOSS = 0.0f;
bool sht31OK     = false;
bool heaterState = false;
bool fanState    = false;
bool exhaustState = false;
unsigned long dryingStartMs = 0;  // millis when drying started

// SoftwareSerial to NodeMCU Bridge (D9=RX, D10=TX)
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

// Line buffers for UART receive (NodeMCU) and Serial Monitor
static String uartRxBuf;
static String serialRxBuf;

// Send full JSON status line to a Stream (ss to NodeMCU; also echo to Serial for debug)
void sendStatus(Stream& out) {
  const char* stateStr;
  switch (systemState) {
    case STATE_DRYING:   stateStr = "DRYING";   break;
    case STATE_COMPLETE: stateStr = "COMPLETE"; break;
    case STATE_PAUSED:   stateStr = "PAUSED";   break;
    default:             stateStr = "IDLE";     break;
  }
  out.print(F("{\"temp\":"));              out.print(CURRENT_TEMPERATURE, 1);
  out.print(F(",\"hum\":"));               out.print(CURRENT_HUMIDITY, 0);
  out.print(F(",\"weight\":"));            out.print(CURRENT_WEIGHT, 2);
  out.print(F(",\"loss\":"));              out.print(CURRENT_WATER_LOSS, 1);
  out.print(F(",\"heater\":"));            out.print(heaterState  ? 1 : 0);
  out.print(F(",\"fan\":"));               out.print(fanState     ? 1 : 0);
  out.print(F(",\"exhaust\":"));           out.print(exhaustState ? 1 : 0);
  out.print(F(",\"pid\":"));               out.print(PID_ENABLED  ? 1 : 0);
  out.print(F(",\"state\":\""));           out.print(stateStr);
  out.print(F("\",\"target_temp\":"));     out.print(TEMPERATURE_SETPOINT, 1);
  out.print(F(",\"target_loss\":"));       out.print(WATER_LOSS_TARGET, 1);
  out.print(F(",\"pid_out\":"));           out.print(PID_OUTPUT, 0);
  out.print(F(",\"runtime\":"));           out.print((systemState == STATE_DRYING)
                                             ? (uint16_t)((millis() - dryingStartMs) / 1000UL) : 0);
  out.print(F(",\"fan_a\":0,\"heater_w\":0,\"bat_v\":0,\"sht31\":"));
  out.print(sht31OK ? 1 : 0);
  out.println(F("}"));
}

void operateSSR(int relayIndex, bool state) {
  switch (relayIndex) {
    case 1: digitalWrite(SSR1_PIN, state ? HIGH : LOW); heaterState  = state; break;
    case 2: digitalWrite(SSR2_PIN, state ? HIGH : LOW); fanState     = state; break;
    case 3: digitalWrite(SSR3_PIN, state ? HIGH : LOW); exhaustState = state; break;
  }
}

// Send a full JSON status line to Serial
void sendStatusJSON() {
  sendStatus(Serial);
}

// Update calculated water loss percentage
void updateWaterLoss() {
  if (INITIAL_WEIGHT > 0.0f) {
    float lost = INITIAL_WEIGHT - CURRENT_WEIGHT;
    if (lost < 0) lost = 0;
    CURRENT_WATER_LOSS = (lost / INITIAL_WEIGHT) * 100.0f;
  } else {
    CURRENT_WATER_LOSS = 0.0f;
  }
}

// Handle one text command line — accepts commands from BOTH NodeMCU UART and Serial Monitor.
//
// NodeMCU protocol  (via SoftwareSerial ss):
//   STATUS?   SETPOINT:<°C>   WATER_LOSS:<%>   PID_START   PID_STOP
//   PAUSE     RESUME          SSR1:1/0          SSR2:1/0    SSR3:1/0
//   TARE      CALIBRATE:<kg>
//
// Serial Monitor style (same as esp/FishDryer):
//   SSR1:1  SSR1:0  SSR2:1  SSR2:0  SSR3:1  SSR3:0
//   SHT31:READ   LOADCELL:READ   STATUS   STATUS?
//   PID:START    PID:STOP    PID:SET:<°C>    PID:READ
//   SETPOINT:<°C>   WATER_LOSS:<%>   TARE   CALIBRATE:<kg>
void handleUARTLine(const String& line) {
  Serial.print(F("[CMD] ")); Serial.println(line);

  // ── Status request ──────────────────────────────────────────────────
  if (line == F("STATUS?") || line == F("STATUS")) {
    sendStatus(ss);
    sendStatus(Serial);

  // ── Sensor reads (Serial Monitor convenience) ───────────────────────
  } else if (line == F("SHT31:READ")) {
    updateSHT31();
    Serial.print(F("Temp: ")); Serial.print(CURRENT_TEMPERATURE, 1); Serial.println(F(" C"));
    Serial.print(F("Hum: "));  Serial.print(CURRENT_HUMIDITY, 0);    Serial.println(F(" %"));

  } else if (line == F("LOADCELL:READ")) {
    CURRENT_WEIGHT = readLoadCell();
    Serial.print(F("Weight: ")); Serial.print(CURRENT_WEIGHT, 3); Serial.println(F(" kg"));

  // ── PID commands ─────────────────────────────────────────────────────
  } else if (line == F("PID:START") || line == F("PID_START") || line == F("STATE:1")) {
    CURRENT_WEIGHT     = readLoadCell();
    INITIAL_WEIGHT     = CURRENT_WEIGHT;
    CURRENT_WATER_LOSS = 0.0f;
    dryingStartMs      = millis();
    PID_ENABLED        = true;
    systemState        = STATE_DRYING;
    Serial.println(F("PID thermostat control ENABLED. System will maintain temperature automatically."));

  } else if (line == F("PID:STOP") || line == F("PID_STOP") || line == F("STATE:0")) {
    PID_ENABLED = false;
    systemState = STATE_IDLE;
    operateSSR(1, false); operateSSR(2, false); operateSSR(3, false);
    Serial.println(F("PID thermostat control DISABLED."));

  } else if (line.startsWith(F("PID:SET:"))) {
    TEMPERATURE_SETPOINT = line.substring(8).toFloat();
    Serial.print(F("Temperature setpoint set to: ")); Serial.println(TEMPERATURE_SETPOINT, 1);

  } else if (line == F("PID:READ")) {
    Serial.print(F("PID Status: "));
    Serial.print(PID_ENABLED ? F("ENABLED") : F("DISABLED"));
    Serial.print(F(" | Current Temp: ")); Serial.print(CURRENT_TEMPERATURE, 1);
    Serial.print(F(" | Setpoint: "));    Serial.print(TEMPERATURE_SETPOINT, 1);
    Serial.print(F(" | PID Output: "));  Serial.println(PID_OUTPUT, 0);

  // ── Setpoints (NodeMCU protocol) ─────────────────────────────────────
  } else if (line.startsWith(F("SETPOINT:"))) {
    TEMPERATURE_SETPOINT = line.substring(9).toFloat();
    Serial.print(F("Temperature setpoint set to: ")); Serial.println(TEMPERATURE_SETPOINT, 1);

  } else if (line.startsWith(F("WATER_LOSS:"))) {
    WATER_LOSS_TARGET = line.substring(11).toFloat();
    Serial.print(F("Water loss target set to: ")); Serial.print(WATER_LOSS_TARGET, 1); Serial.println(F(" %"));

  // ── Drying session control ────────────────────────────────────────────
  } else if (line == F("PAUSE") || line == F("STATE:3")) {
    PID_ENABLED = false;
    systemState = STATE_PAUSED;
    Serial.println(F("Drying PAUSED."));

  } else if (line == F("RESUME")) {
    if (systemState == STATE_PAUSED) {
      PID_ENABLED = true;
      systemState = STATE_DRYING;
      Serial.println(F("Drying RESUMED."));
    }

  // ── SSR / relay direct control ────────────────────────────────────────
  } else if (line == F("SSR1:1")) {
    operateSSR(1, true);  Serial.println(F("SSR1_PIN set HIGH"));
  } else if (line == F("SSR1:0")) {
    operateSSR(1, false); Serial.println(F("SSR1_PIN set LOW"));
  } else if (line == F("SSR2:1")) {
    operateSSR(2, true);  Serial.println(F("SSR2_PIN set HIGH"));
  } else if (line == F("SSR2:0")) {
    operateSSR(2, false); Serial.println(F("SSR2_PIN set LOW"));
  } else if (line == F("SSR3:1")) {
    operateSSR(3, true);  Serial.println(F("SSR3_PIN set HIGH"));
  } else if (line == F("SSR3:0")) {
    operateSSR(3, false); Serial.println(F("SSR3_PIN set LOW"));

  // ── Load cell helpers ─────────────────────────────────────────────────
  } else if (line == F("TARE")) {
    tareLoadCell();

  } else if (line.startsWith(F("CALIBRATE:"))) {
    calibrateLoadCell(line.substring(10).toFloat());

  } else {
    Serial.println(F("Unknown command. Available: SSR1:1, SSR1:0, SSR2:1, SSR2:0, SSR3:1, SSR3:0,"));
    Serial.println(F("  SHT31:READ, LOADCELL:READ, STATUS, STATUS?,"));
    Serial.println(F("  PID:START, PID:STOP, PID:SET:<°C>, PID:READ,"));
    Serial.println(F("  SETPOINT:<°C>, WATER_LOSS:<%, PAUSE, RESUME, TARE, CALIBRATE:<kg>"));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(SSR1_PIN, OUTPUT);
  pinMode(SSR2_PIN, OUTPUT);
  pinMode(SSR3_PIN, OUTPUT);
  Serial.println(F("SSR_CONFIG loaded. SSR1_PIN, SSR2_PIN, and SSR3_PIN set as OUTPUT."));

  // SHT31 init — uses SoftwareWire (D7/D8)
  initSHT31();
  initLoadCell();
  initPID();
  Serial.println(F("PID controller initialized."));

  // SoftwareSerial to NodeMCU Bridge (D9=RX, D10=TX)
  uartRxBuf.reserve(128);
  serialRxBuf.reserve(128);
  ss.begin(SS_UART_BAUD);

  Serial.print(F("READY  UART D9(RX)/D10(TX) baud="));
  Serial.println(SS_UART_BAUD);
  Serial.println(F("Type a command and press Enter in Serial Monitor."));
}

void loop() {
  // PID thermostat control (non-blocking)
  if (millis() - lastPIDUpdate >= 2000UL) {
    updateSHT31();
    CURRENT_WEIGHT = readLoadCell();
    updateWaterLoss();

    if (PID_ENABLED) {
      pidCOMPUTE();
    }

    // (status is sent on demand when NodeMCU sends STATUS?)
    lastPIDUpdate = millis();

    // Auto-stop when water loss target is reached
    if (systemState == STATE_DRYING && WATER_LOSS_TARGET > 0 &&
        CURRENT_WATER_LOSS >= WATER_LOSS_TARGET) {
      PID_ENABLED = false;
      systemState = STATE_COMPLETE;
      operateSSR(1, false); operateSSR(2, false); operateSSR(3, false);
      Serial.println(F("DRYING_COMPLETE"));
    }
  }

  // Process commands from NodeMCU Bridge (SoftwareSerial D9/D10)
  while (ss.available()) {
    char c = (char)ss.read();
    if (c == '\n') {
      uartRxBuf.trim();
      if (uartRxBuf.length() > 0) {
        handleUARTLine(uartRxBuf);
      }
      uartRxBuf = "";
    } else if (c != '\r') {
      if (uartRxBuf.length() < 120) uartRxBuf += c;
    }
  }

  // Process commands from Serial Monitor (USB Serial)
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      serialRxBuf.trim();
      if (serialRxBuf.length() > 0) {
        handleUARTLine(serialRxBuf);
      }
      serialRxBuf = "";
    } else if (c != '\r') {
      if (serialRxBuf.length() < 120) serialRxBuf += c;
    }
  }
}
