/**
 * NanoUartSample.ino - Arduino Nano as UART Slave  [plain-text / test]
 *
 * Plain-text protocol (newline-terminated) for easy testing with a serial
 * monitor.  All messages are human-readable strings.
 *
 * NodeMCU -> Nano commands:
 *   "SETPOINT:65.00"  - set temperature setpoint
 *   "STATE:1"         - set dryer state (0=idle, 1=drying, 2=done)
 *   "STATUS?"         - request status
 *
 * Nano -> NodeMCU reply:
 *   "STATUS:T=42.50,H=55.00,W=1200.00,S=0,F=1"
 *
 * -- Hardware ------------------------------------------------------------------
 *   Board          : Arduino Nano (ATmega328P, 16 MHz, 5 V logic)
 *   SoftwareSerial : RX = D9, TX = D10
 *   Baud           : 115200
 *
 * -- Wiring --------------------------------------------------------------------
 *   Nano D9  (SoftwareSerial RX) --wire--> NodeMCU D6 (GPIO12, TX)
 *   Nano D10 (SoftwareSerial TX) --wire--> NodeMCU D5 (GPIO14, RX)
 *   Nano GND                     --wire--> NodeMCU GND
 *
 * -- WARNING: NO LEVEL SHIFTER -------------------------------------------------
 *   Nano D10 TX outputs 5 V logic.  NodeMCU D5 (GPIO14) is 3.3 V max.
 *   Use a voltage divider on that line only:
 *
 *     Nano D10 (5 V) --[10 kOhm]--+---> NodeMCU D5 (GPIO14)
 *                                  |
 *                               [20 kOhm]      ~3.33 V OK
 *                                  |
 *                                 GND
 *
 * -- BAUD RATE NOTE ------------------------------------------------------------
 *   115200 baud on SoftwareSerial at 16 MHz is at the upper limit of
 *   reliability.  Keep lines short; if corruption occurs, drop to 57600.
 */

#include <SoftwareSerial.h>

// -- SoftwareSerial pin assignments --------------------------------------------
#define SS_RX_PIN   9       // Nano D9  <- NodeMCU D6 (TX)
#define SS_TX_PIN   10      // Nano D10 -> NodeMCU D5 (RX)
#define UART_BAUD   9600

// -- Runtime state -------------------------------------------------------------
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

static float   currentSetpoint = 60.0f;
static uint8_t currentState    = 0;     // 0 = idle

// -- Send status reply: "STATUS:T=xx.xx,H=xx.xx,W=xxxx.xx,S=x,F=x" -----------
static void sendStatus()
{
    // TODO: replace stub values with real SHT31 / HX711 readings.
    char buf[64];
    snprintf(buf, sizeof(buf),
             "STATUS:T=42.50,H=55.00,W=1200.00,S=%d,F=1",
             currentState);
    ss.println(buf);
    Serial.print(F("[Nano] TX: "));
    Serial.println(buf);
}

// -- Handle one received text line ---------------------------------------------
static void handleLine(String line)
{
    line.trim();    // remove \r or leading/trailing spaces

    if (line.length() == 0) return;

    Serial.print(F("[Nano] RX: "));
    Serial.println(line);

    if (line == "STATUS?") {
        sendStatus();

    } else if (line.startsWith("SETPOINT:")) {
        currentSetpoint = line.substring(9).toFloat();
        Serial.print(F("[Nano] Setpoint set to "));
        Serial.print(currentSetpoint);
        Serial.println(F(" C"));

    } else if (line.startsWith("STATE:")) {
        currentState = (uint8_t)line.substring(6).toInt();
        Serial.print(F("[Nano] State set to "));
        Serial.println(currentState);

    } else {
        Serial.print(F("[Nano] Unknown command: "));
        Serial.println(line);
    }
}

// -----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println(F("[Nano] Plain-text UART Slave starting..."));

    ss.begin(UART_BAUD);
    Serial.print(F("[Nano] SoftwareSerial ready  RX=D"));
    Serial.print(SS_RX_PIN);
    Serial.print(F("  TX=D"));
    Serial.print(SS_TX_PIN);
    Serial.print(F("  baud="));
    Serial.println(UART_BAUD);
}

void loop()
{
    if (ss.available()) {
        String line = ss.readStringUntil('\n');
        handleLine(line);
    }
}
