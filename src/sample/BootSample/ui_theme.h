// ui_theme.h - Minimal theme for standalone bootscreen sample

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// =========================
// Display dimensions
// =========================
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  480

// =========================
// Colors - Industrial Dark Theme
// =========================
#define COLOR_PRIMARY       lv_color_hex(0x0A2540)  // Deep blue
#define COLOR_ACCENT        lv_color_hex(0xFF6B00)  // Heat orange
#define COLOR_SUCCESS       lv_color_hex(0x00D084)  // Green
#define COLOR_BG            lv_color_hex(0x0F172A)  // Dark background
#define COLOR_BG_CARD       lv_color_hex(0x1E293B)  // Card background
#define COLOR_TEXT_PRIMARY  lv_color_hex(0xFFFFFF)  // White
#define COLOR_TEXT_SECONDARY lv_color_hex(0x94A3B8) // Gray

// =========================
// Font references
// =========================
#define FONT_SMALL      &lv_font_montserrat_16
#define FONT_MEDIUM     &lv_font_montserrat_20
#define FONT_XXL        &lv_font_montserrat_36
#define FONT_HUGE       &lv_font_montserrat_48

// =========================
// Animation durations (ms)
// =========================
#define ANIM_BOOT_FADE      500

#endif // UI_THEME_H
