// BootSample.ino
// Standalone bootscreen demonstration for Fish Dryer V2 HMI
// 
// This sketch displays a complete bootscreen animation with:
// - Custom logo image from SD card (solaraw.png)
// - Pulsing icon animation
// - Fading title
// - Animated progress bar
// - Status text updates
//
// Hardware: Waveshare ESP32-S3-Touch-LCD-7 (800x480)
// Framework: LVGL v8
//
// Required LVGL fonts (enable in lv_conf.h):
//   LV_FONT_MONTSERRAT_16, _20, _36, _48

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "ui_theme.h"
#include "ui_styles.h"
#include "boot_screen.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// SD card pin definitions
#define SD_MOSI 11
#define SD_CLK  12
#define SD_MISO 13
#define SD_SS   14

// LVGL timer callback for periodic updates
static void uiUpdateTimerCb(lv_timer_t* timer) {
    (void)timer;
    // Periodic UI update (called every 50ms)
}

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for serial connection
    
    Serial.println("\n\n=== Boot Screen Sample with SD Card Logo ===");
    
    // ========== Initialize SD Card ==========
    Serial.println("[BOOT] Initializing SD card...");
    SPI.setHwCs(false);
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_SS);
    
    if (!SD.begin(SD_SS)) {
        Serial.println("[BOOT] WARNING: SD card mount failed - will use fallback symbol");
    } else {
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("[BOOT] WARNING: No SD card detected");
        } else {
            Serial.print("[BOOT] SD Card Type: ");
            if (cardType == CARD_MMC) {
                Serial.println("MMC");
            } else if (cardType == CARD_SD) {
                Serial.println("SDSC");
            } else if (cardType == CARD_SDHC) {
                Serial.println("SDHC");
            } else {
                Serial.println("UNKNOWN");
            }
            uint64_t cardSize = SD.cardSize() / (1024 * 1024);
            Serial.printf("[BOOT] SD Card Size: %lluMB\n", cardSize);
        }
    }

    // ========== Initialize Display ==========
    Serial.println("[BOOT] Initializing display board...");
    Board *board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("[BOOT] Initializing LVGL...");
    lvgl_port_init(board->getLCD(), board->getTouch());

    // Build the UI (must hold LVGL mutex)
    Serial.println("[BOOT] Building UI...");
    lvgl_port_lock(-1);

    // Initialize styles
    initStyles();

    // Create and display boot screen with SD card support
    lv_obj_t* bootScr = createBootScreen(&SD);
    if (bootScr) {
        lv_scr_load(bootScr);
        Serial.println("[BOOT] Boot screen displayed");
    } else {
        Serial.println("[BOOT] ERROR: Failed to create boot screen!");
    }

    // Create periodic timer for UI updates (every 50ms)
    lv_timer_create(uiUpdateTimerCb, 50, NULL);

    lvgl_port_unlock();

    Serial.println("[BOOT] Setup complete!");
    Serial.printf("[BOOT] Free heap: %lu KB\n", (unsigned long)(ESP.getFreeHeap() / 1024));
    Serial.println("[BOOT] Boot screen will display for 3 seconds with animations...\n");
}

void loop() {
    // Main loop - LVGL handles rendering via timer task
    delay(50);
}
