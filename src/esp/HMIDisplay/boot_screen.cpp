// boot_screen.cpp
// Fish Dryer V2 HMI - Boot/splash screen with animations

#include "boot_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "screen_manager.h"

static lv_obj_t* boot_bar = NULL;
static lv_obj_t* boot_status_label = NULL;
static lv_anim_t boot_bar_anim;

static const char* bootMessages[] = {
    "Initializing sensors...",
    "Connecting to controller...",
    "Loading configuration...",
    "Starting system...",
    "Ready"
};

// Timer callback: transition to dashboard after boot.
// NOTE: Do NOT call lv_timer_del() here. The timer is created with
// repeat_count = 1 (one-shot), so LVGL deletes it safely after this
// callback returns. Calling lv_timer_del() from inside its own callback
// causes a use-after-free in lv_timer_handler() → LoadProhibited crash.
static void bootCompleteCallback(lv_timer_t* timer) {
    (void)timer;  // LVGL handles deletion via repeat_count = 1
    loadScreen(SCREEN_DASHBOARD);
}

// Animation callback: update progress bar value
static void bootBarAnimCb(void* var, int32_t v) {
    lv_bar_set_value((lv_obj_t*)var, v, LV_ANIM_OFF);

    // Update status text based on progress
    if (boot_status_label) {
        int idx = v / 25;
        if (idx > 4) idx = 4;
        lv_label_set_text(boot_status_label, bootMessages[idx]);
    }
}

// Heat icon pulse animation callback
static void pulseAnimCb(void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}

lv_obj_t* createBootScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // ---- Fish icon (using LVGL symbol as placeholder) ----
    lv_obj_t* icon = lv_label_create(scr);
    lv_label_set_text(icon, LV_SYMBOL_CHARGE);  // Lightning bolt = heat symbol
    lv_obj_set_style_text_font(icon, FONT_HUGE, 0);
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -100);

    // Pulse animation on icon
    lv_anim_t pulse;
    lv_anim_init(&pulse);
    lv_anim_set_var(&pulse, icon);
    lv_anim_set_values(&pulse, 80, 255);
    lv_anim_set_time(&pulse, 800);
    lv_anim_set_playback_time(&pulse, 800);
    lv_anim_set_repeat_count(&pulse, 3);
    lv_anim_set_exec_cb(&pulse, pulseAnimCb);
    lv_anim_start(&pulse);

    // ---- Title ----
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "FISH DRYER V2");
    lv_obj_set_style_text_font(title, FONT_XXL, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_letter_space(title, 4, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);

    // Fade-in animation for title
    lv_anim_t fadeIn;
    lv_anim_init(&fadeIn);
    lv_anim_set_var(&fadeIn, title);
    lv_anim_set_values(&fadeIn, 0, 255);
    lv_anim_set_time(&fadeIn, ANIM_BOOT_FADE);
    lv_anim_set_exec_cb(&fadeIn, pulseAnimCb);
    lv_anim_start(&fadeIn);

    // ---- Subtitle ----
    lv_obj_t* subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "Smart Drying System");
    lv_obj_set_style_text_font(subtitle, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 15);

    // ---- Progress bar ----
    boot_bar = lv_bar_create(scr);
    lv_obj_set_size(boot_bar, 400, 8);
    lv_obj_align(boot_bar, LV_ALIGN_CENTER, 0, 75);
    lv_bar_set_range(boot_bar, 0, 100);
    lv_bar_set_value(boot_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(boot_bar, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_color(boot_bar, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(boot_bar, 4, 0);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_INDICATOR);

    // Animate progress bar from 0 to 100 over boot duration
    lv_anim_init(&boot_bar_anim);
    lv_anim_set_var(&boot_bar_anim, boot_bar);
    lv_anim_set_values(&boot_bar_anim, 0, 100);
    lv_anim_set_time(&boot_bar_anim, 2800);
    lv_anim_set_exec_cb(&boot_bar_anim, bootBarAnimCb);
    lv_anim_start(&boot_bar_anim);

    // ---- Status label ----
    boot_status_label = lv_label_create(scr);
    lv_label_set_text(boot_status_label, "Initializing...");
    lv_obj_set_style_text_font(boot_status_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(boot_status_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(boot_status_label, LV_ALIGN_CENTER, 0, 100);

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
