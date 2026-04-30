// BootSample.ino
// Standalone bootscreen demonstration for Fish Dryer V2 HMI
// 
// This sketch displays a complete bootscreen animation with:
// - Custom logo image from compiled C array (solaraw.c)
// - Static logo (no icon pulse animation)
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
#include "esp_panel_board_custom_conf.h"
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

#include "ui_theme.h"
#include "ui_styles.h"
#include "boot_screen.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// LVGL timer for periodic UI updates
static void uiUpdateTimerCb(lv_timer_t* timer) {
    (void)timer;
    // Periodic UI update (called every 50ms)
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== Boot Screen Sample with Compiled Logo ===");

    // Initialize display board
    Serial.println("[HMI] Initializing display board...");
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

    // Initialize LVGL
    Serial.println("[HMI] Initializing LVGL...");
    lvgl_port_init(board->getLCD(), board->getTouch());

    // Initialize boot screen flow
    Serial.println("[HMI] Initializing boot screen...");

    // Build the UI (must hold LVGL mutex)
    Serial.println("[HMI] Building UI...");
    lvgl_port_lock(-1);

    // Initialize styles and theme
    initStyles();

    // Create and display boot screen with compiled image support
    lv_obj_t* bootScr = createBootScreen();
    if (bootScr) {
        lv_scr_load(bootScr);
        Serial.println("[HMI] Screen Loaded: BOOT");
    } else {
        Serial.println("[HMI] ERROR: Failed to create boot screen!");
    }

    // Create periodic timer for UI updates
    lv_timer_create(uiUpdateTimerCb, 50, NULL);

    lvgl_port_unlock();

    Serial.println("[HMI] Initialization complete!");
    Serial.printf("[HMI] Free heap: %lu KB\n", (unsigned long)(ESP.getFreeHeap() / 1024));
    Serial.println("[HMI] Boot screen will display for 3 seconds with animations...\n");
}

void loop() {
    // Handle UI updates (non-blocking)
    delay(50);
}
