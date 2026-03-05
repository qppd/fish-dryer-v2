// HMIDisplay.ino
// Fish Dryer V2 - HMI Display Controller
// Industrial-grade control panel for automated drying systems
//
// Hardware: Waveshare ESP32-S3-Touch-LCD-7 (800x480)
// Framework: LVGL v8
// Communication: UART JSON protocol to FishDryer controller
//
// Required LVGL fonts (enable in lv_conf.h):
//   LV_FONT_MONTSERRAT_14, _16, _20, _24, _30, _36, _48
// Required LVGL widgets (enable in lv_conf.h):
//   LV_USE_METER, LV_USE_CHART, LV_USE_BAR, LV_USE_SLIDER,
//   LV_USE_SWITCH, LV_USE_TABVIEW, LV_USE_CHECKBOX, LV_USE_SPINNER

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

#include "dryer_data.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "serial_protocol.h"
#include "screen_manager.h"
#include "alert_popup.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// Global dryer data instance
DryerData dryerData;

// LVGL timer for periodic UI updates
static void uiUpdateTimerCb(lv_timer_t* timer) {
    (void)timer;
    updateCurrentScreen();
    checkAlerts();
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== Fish Dryer V2 HMI ===");

    // Initialize dryer data with defaults
    initDryerData();

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

    // Initialize UART communication with dryer controller
    Serial.println("[HMI] Initializing serial protocol...");
    serialProtoInit();

    // Build the UI (must hold LVGL mutex)
    Serial.println("[HMI] Building UI...");
    lvgl_port_lock(-1);

    // Initialize styles and theme
    initStyles();

    // Initialize alert system
    alertInit();

    // Initialize screen manager (creates and shows boot screen)
    screenManagerInit();

    // Create periodic timer for UI data updates (every 2 seconds)
    lv_timer_create(uiUpdateTimerCb, CHART_UPDATE_MS, NULL);

    lvgl_port_unlock();

    Serial.println("[HMI] Initialization complete!");
    Serial.printf("[HMI] Free heap: %lu KB\n", (unsigned long)(ESP.getFreeHeap() / 1024));
}

void loop() {
    // Handle UART communication with dryer controller (non-blocking)
    serialProtoUpdate();

    delay(10);
}
