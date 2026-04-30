// boot_screen.cpp
// Standalone bootscreen implementation for Fish Dryer V2 HMI
// Uses compiled logo image from C array

#include <Arduino.h>
#include "boot_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "solaraw.h"

// Timer callback: boot complete
static void bootCompleteCallback(lv_timer_t* timer) {
    (void)timer;
    Serial.println("[BOOT] Boot sequence complete!");
}

lv_obj_t* createBootScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // ---- Logo/Icon (Image from SD card or fallback to symbol) ----
    lv_obj_t* icon = NULL;
    icon = lv_img_create(scr);
    lv_img_set_src(icon, &solaraw);
    lv_obj_center(icon);
    // Move up from center
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -100);
    Serial.println("[BOOT] Using compiled solaraw image (C array)");

    // ---- Title ----
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "SolAraw");
    lv_obj_set_style_text_font(title, FONT_XXL, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_letter_space(title, 4, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);

    // ---- Subtitle ----
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

    // ---- Timer for boot complete callback (3 second display) ----
    lv_timer_t* boot_timer = lv_timer_create(bootCompleteCallback, 3000, NULL);
    lv_timer_set_repeat_count(boot_timer, 1);

    return scr;
}
