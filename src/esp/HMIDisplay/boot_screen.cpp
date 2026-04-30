// boot_screen.cpp
// Fish Dryer V2 HMI - Boot/splash screen with animations

#include <Arduino.h>
#include "boot_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "screen_manager.h"
#include "solaraw.h"

// Timer callback: transition to dashboard after boot.
// NOTE: Do NOT call lv_timer_del() here. The timer is created with
// repeat_count = 1 (one-shot), so LVGL deletes it safely after this
// callback returns. Calling lv_timer_del() from inside its own callback
// causes a use-after-free in lv_timer_handler() → LoadProhibited crash.
static void bootCompleteCallback(lv_timer_t* timer) {
    (void)timer;  // LVGL handles deletion via repeat_count = 1
    loadScreen(SCREEN_DASHBOARD);
}

lv_obj_t* createBootScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // ---- Logo image (compiled C array) ----
    lv_obj_t* icon = lv_img_create(scr);
    lv_img_set_src(icon, &solaraw);
    lv_img_set_zoom(icon, 170);  // ~66% scale (256 = 100%)
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -100);
    Serial.println("[BOOT] Using compiled solaraw image (C array)");

    // ---- Subtitle (static) ----
    lv_obj_t* subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Smart Drying System");
    lv_obj_set_style_text_font(subtitle, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 15);

    // ---- Static status label (no animation) ----
    lv_obj_t* status = lv_label_create(scr);
    lv_label_set_text(status, "Initializing...");
    lv_obj_set_style_text_font(status, FONT_SMALL, 0);
    lv_obj_set_style_text_color(status, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 100);

    // ---- Version info ----
    lv_obj_t* version = lv_label_create(scr);
    lv_label_set_text(version, "v2.0.0");
    lv_obj_set_style_text_font(version, FONT_SMALL, 0);
    lv_obj_set_style_text_color(version, lv_color_hex(0x475569), 0);
    lv_obj_align(version, LV_ALIGN_BOTTOM_MID, 0, -15);

    // ---- Timer to transition to dashboard ----
    // repeat_count = 1 makes this a one-shot timer; LVGL deletes it after
    // the first fire, so the callback does NOT need to call lv_timer_del().
    lv_timer_t* boot_timer = lv_timer_create(bootCompleteCallback, 3000, NULL);
    lv_timer_set_repeat_count(boot_timer, 1);

    return scr;
}
