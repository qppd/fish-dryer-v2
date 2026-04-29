// screen_manager.cpp
// Fish Dryer V2 HMI - Screen navigation implementation

#include <Arduino.h>
#include "screen_manager.h"
#include "ui_theme.h"
#include "boot_screen.h"
#include "dashboard_screen.h"
#include "control_screen.h"
#include "analytics_screen.h"
#include "diagnostics_screen.h"

static ScreenId currentScreen = SCREEN_BOOT;
static ScreenId previousScreen = SCREEN_DASHBOARD;
static lv_obj_t* screens[SCREEN_COUNT] = { NULL };

// LVGL port functions for thread safety (defined in lvgl_v8_port.cpp)
extern void lvgl_port_lock(int timeout);
extern void lvgl_port_unlock();

// Screen creation functions
static lv_obj_t* createScreen(ScreenId id) {
    switch (id) {
        case SCREEN_BOOT:        return createBootScreen();
        case SCREEN_DASHBOARD:   return createDashboardScreen();
        case SCREEN_CONTROL:     return createControlScreen();
        case SCREEN_ANALYTICS:   return createAnalyticsScreen();
        case SCREEN_DIAGNOSTICS: return createDiagnosticsScreen();
        default: return NULL;
    }
}

void screenManagerInit() {
    // Create and show boot screen first
    lvgl_port_lock(-1);
    screens[SCREEN_BOOT] = createScreen(SCREEN_BOOT);
    if (screens[SCREEN_BOOT]) {
        lv_scr_load(screens[SCREEN_BOOT]);
        currentScreen = SCREEN_BOOT;
        Serial.println("[HMI] Screen Loaded: BOOT");
    }
    lvgl_port_unlock();
}

void loadScreen(ScreenId id) {
    if (id == currentScreen || id >= SCREEN_COUNT) return;

    lvgl_port_lock(-1);
    
    previousScreen = currentScreen;

    // Create screen if not yet created
    if (screens[id] == NULL) {
        screens[id] = createScreen(id);
        if (!screens[id]) {
            lvgl_port_unlock();
            return;  // Screen creation failed
        }
    }

    // For the one-time boot screen, pass auto_del = true so LVGL removes
    // disp->prev_scr from its internal state *after* the animation ends.
    // This avoids a dangling prev_scr pointer during the fade-in frames.
    // For all other screens keep auto_del = false so they remain allocated
    // and can be revisited instantly without re-creation.
    bool auto_del = (previousScreen == SCREEN_BOOT);
    
    if (screens[id]) {
        lv_scr_load_anim(screens[id], LV_SCR_LOAD_ANIM_FADE_ON, ANIM_SCREEN_TRANS, 0, auto_del);
        currentScreen = id;
        
        // Log screen load for debugging
        const char* screenName = "";
        switch (id) {
            case SCREEN_BOOT:        screenName = "BOOT"; break;
            case SCREEN_DASHBOARD:   screenName = "DASHBOARD"; break;
            case SCREEN_CONTROL:     screenName = "CONTROL"; break;
            case SCREEN_ANALYTICS:   screenName = "ANALYTICS"; break;
            case SCREEN_DIAGNOSTICS: screenName = "DIAGNOSTICS"; break;
            default:                 screenName = "UNKNOWN"; break;
        }
        Serial.print("[HMI] Screen Loaded: ");
        Serial.println(screenName);
    }

    // Mark boot screen pointer as gone; LVGL owns the object now (auto_del).
    if (previousScreen == SCREEN_BOOT) {
        screens[SCREEN_BOOT] = NULL;
    }

    lvgl_port_unlock();
}

ScreenId getCurrentScreen() {
    return currentScreen;
}

void updateCurrentScreen() {
    // Wrap UI updates with LVGL lock for thread safety
    lvgl_port_lock(-1);
    
    switch (currentScreen) {
        case SCREEN_DASHBOARD:   updateDashboardScreen(); break;
        case SCREEN_CONTROL:     updateControlScreen(); break;
        case SCREEN_ANALYTICS:   updateAnalyticsScreen(); break;
        case SCREEN_DIAGNOSTICS: updateDiagnosticsScreen(); break;
        default: break;
    }
    
    lvgl_port_unlock();
}

void backBtnCallback(lv_event_t* e) {
    (void)e;
    loadScreen(SCREEN_DASHBOARD);
}
