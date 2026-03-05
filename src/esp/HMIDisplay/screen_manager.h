// screen_manager.h
// Fish Dryer V2 HMI - Screen navigation and lifecycle management

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>

// Screen identifiers
enum ScreenId {
    SCREEN_BOOT,
    SCREEN_DASHBOARD,
    SCREEN_CONTROL,
    SCREEN_ANALYTICS,
    SCREEN_DIAGNOSTICS,
    SCREEN_COUNT
};

// Initialize the screen manager (call after LVGL + styles init)
void screenManagerInit();

// Navigate to a screen with animation
void loadScreen(ScreenId id);

// Get current screen ID
ScreenId getCurrentScreen();

// Update current screen data (call from LVGL timer)
void updateCurrentScreen();

// Back button handler
void backBtnCallback(lv_event_t* e);

#endif // SCREEN_MANAGER_H
