/**
 * NODEMCUUartSample.ino — NodeMCU (ESP8266) as UART Master  [plain-text / test]
 *
 * Plain-text protocol (newline-terminated) for easy testing with a serial
 * monitor.  All messages are human-readable strings.
 *
 * NodeMCU → Nano commands (sent as text lines):
 *   "SETPOINT:65.00"  — set temperature setpoint
 *   "STATE:1"         — set dryer state (0=idle, 1=drying, 2=done)
 *   "STATUS?"         — request status from Nano
 *
 * Nano → NodeMCU reply:
 *   "STATUS:T=42.50,H=55.00,W=1200.00,S=0,F=1"
 *
 * ── Hardware ──────────────────────────────────────────────────────────────────
 *   Board          : NodeMCU 1.0 (ESP-12E / ESP8266, 3.3 V logic)
 *   SoftwareSerial : TX = D6 (GPIO12), RX = D5 (GPIO14)
 *   Baud           : 115200
 *
 * ── Wiring ────────────────────────────────────────────────────────────────────
 *   NodeMCU D6 (GPIO12, TX) ──wire──> Nano D9  (SoftwareSerial RX)
 *   NodeMCU D5 (GPIO14, RX) ──wire──> Nano D10 (SoftwareSerial TX)
 *   NodeMCU GND             ──wire──> Nano GND
 *
 * ── ⚠ VOLTAGE WARNING — NO LEVEL SHIFTER ─────────────────────────────────────
 *   Nano D10 TX outputs 5 V logic.  NodeMCU D5 (GPIO14) is 3.3 V max.
 *   Use a voltage divider on that line only:
 *
 *     Nano D10 (5 V) ──[10 kΩ]──┬──> NodeMCU D5 (GPIO14)
 *                                |
 *                             [20 kΩ]       ≈ 3.33 V ✓
 *                                |
 *                               GND
 *
 * ── Required library ─────────────────────────────────────────────────────────
 *   Install "EspSoftwareSerial" (by plerup) from the Arduino Library Manager.
 */

#include <Arduino.h>
#include <SoftwareSerial.h>

// ── SoftwareSerial pin assignments ────────────────────────────────────────────
#define SS_TX_PIN           D6      // GPIO12 → Nano D9  (SS RX)
#define SS_RX_PIN           D5      // GPIO14 ← Nano D10 (SS TX)
#define UART_BAUD           9600

// ── Timing ────────────────────────────────────────────────────────────────────
#define POLL_INTERVAL_MS    1000    // How often to send STATUS?
#define CMD_INTERVAL_MS     5000    // How often to send a control command
#define RESPONSE_TIMEOUT_MS 500     // Max wait for Nano reply

// ── Runtime state ─────────────────────────────────────────────────────────────
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

static unsigned long lastPollTime = 0;
static unsigned long lastCmdTime  = 0;

// ── Send a plain-text command line ────────────────────────────────────────────
static void sendCommand(const String &cmd)
{
    ss.println(cmd);                         // appends \r\n
    Serial.print(F("[NodeMCU] TX: "));
    Serial.println(cmd);
}

// ── Wait for a reply line from the Nano ───────────────────────────────────────
static bool receiveReply(String &out)
{
    ss.setTimeout(RESPONSE_TIMEOUT_MS);
    out = ss.readStringUntil('\n');
    out.trim();                              // remove trailing \r if present

    if (out.length() == 0) {
        Serial.println(F("[NodeMCU] Timeout: no reply from Nano"));
        return false;
    }
    return true;
}

// ── Parse "STATUS:T=42.50,H=55.00,W=1200.00,S=0,F=1" ─────────────────────────
static void parseAndPrintStatus(const String &line)
{
    if (!line.startsWith("STATUS:")) {
        Serial.print(F("[NodeMCU] Unexpected reply: "));
        Serial.println(line);
        return;
    }

    // Extract each field by finding its label and the following comma or EOL.
    auto extract = [&](const char *label) -> String {
        int idx = line.indexOf(label);
        if (idx < 0) return "?";
        idx += strlen(label);
        int end = line.indexOf(',', idx);
        return (end < 0) ? line.substring(idx) : line.substring(idx, end);
    };

    Serial.printf("[NodeMCU] T=%s°C  H=%s%%  W=%sg  State=%s  Flags=%s\n",
                  extract("T=").c_str(),
                  extract("H=").c_str(),
                  extract("W=").c_str(),
                  extract("S=").c_str(),
                  extract("F=").c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println(F("[NodeMCU] Plain-text UART Master — NodeMCU 1.0 (ESP8266)"));

    ss.begin(UART_BAUD);
    Serial.printf("[NodeMCU] SoftwareSerial ready  TX=D6(GPIO12)  RX=D5(GPIO14)  baud=%d\n",
                  UART_BAUD);
}

void loop()
{
    unsigned long now = millis();

    // ── Periodic control command ──────────────────────────────────────────────
    if (now - lastCmdTime >= CMD_INTERVAL_MS) {
        lastCmdTime = now;
        static bool toggle = false;
        if (toggle) {
            sendCommand("SETPOINT:65.00");   // 65 °C target
        } else {
            sendCommand("STATE:1");           // start drying
        }
        toggle = !toggle;
    }

    // ── Periodic status poll ──────────────────────────────────────────────────
    if (now - lastPollTime >= POLL_INTERVAL_MS) {
        lastPollTime = now;
        while (ss.available()) ss.read();    // flush stale bytes
        sendCommand("STATUS?");

        String reply;
        if (receiveReply(reply)) {
            parseAndPrintStatus(reply);
        }
    }
}
