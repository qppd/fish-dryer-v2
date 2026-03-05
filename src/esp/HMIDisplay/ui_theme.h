// ui_theme.h
// Fish Dryer V2 HMI - Visual theme definitions
// Industrial-grade color scheme inspired by lv_demo_music

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// =========================
// Display dimensions
// =========================
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  480

// =========================
// Layout constants
// =========================
#define TOP_BAR_HEIGHT     50
#define BOTTOM_BAR_HEIGHT  60
#define CONTENT_HEIGHT     (SCREEN_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT)
#define SIDE_PADDING       15
#define WIDGET_SPACING     10
#define BTN_MIN_SIZE       48

// =========================
// Colors - Industrial Dark Theme
// =========================
#define COLOR_PRIMARY       lv_color_hex(0x0A2540)  // Deep blue
#define COLOR_ACCENT        lv_color_hex(0xFF6B00)  // Heat orange
#define COLOR_SUCCESS       lv_color_hex(0x00D084)  // Green - normal operation
#define COLOR_DANGER        lv_color_hex(0xFF3B30)  // Red - error/emergency
#define COLOR_WARNING       lv_color_hex(0xFFCC00)  // Yellow - warning
#define COLOR_IDLE          lv_color_hex(0x007AFF)  // Blue - idle state
#define COLOR_HEATING       lv_color_hex(0xFF6B00)  // Orange - heating

#define COLOR_BG            lv_color_hex(0x0F172A)  // Dark background
#define COLOR_BG_CARD       lv_color_hex(0x1E293B)  // Card background
#define COLOR_BG_TOP_BAR    lv_color_hex(0x0A1628)  // Top bar background
#define COLOR_BG_BUTTON     lv_color_hex(0x334155)  // Button background

#define COLOR_TEXT_PRIMARY  lv_color_hex(0xFFFFFF)  // White
#define COLOR_TEXT_SECONDARY lv_color_hex(0x94A3B8) // Gray
#define COLOR_TEXT_ACCENT   lv_color_hex(0xFF6B00)  // Orange text

#define COLOR_BORDER        lv_color_hex(0x334155)  // Subtle border

// Gauge colors
#define COLOR_GAUGE_SAFE    lv_color_hex(0x00D084)  // 0-70°C green
#define COLOR_GAUGE_HOT     lv_color_hex(0xFF6B00)  // 70-90°C orange
#define COLOR_GAUGE_DANGER  lv_color_hex(0xFF3B30)  // 90-120°C red
#define COLOR_GAUGE_BG      lv_color_hex(0x1E293B)  // Gauge background

// Chart colors
#define COLOR_CHART_TEMP    lv_color_hex(0xFF6B00)  // Orange for temperature
#define COLOR_CHART_HUM     lv_color_hex(0x007AFF)  // Blue for humidity
#define COLOR_CHART_WEIGHT  lv_color_hex(0x00D084)  // Green for weight

// =========================
// Font references
// =========================
// Balanced font set enabled in lv_conf.h (~60-80 KB flash, safe for ESP32):
//   16  = labels, status text
//   20  = buttons
//   24  = card titles
//   30  = medium values
//   36  = large metrics (humidity, weight)
//   48  = dashboard temperature

#define FONT_SMALL      &lv_font_montserrat_16  // labels, status text
#define FONT_NORMAL     &lv_font_montserrat_16  // secondary labels
#define FONT_MEDIUM     &lv_font_montserrat_20
#define FONT_LARGE      &lv_font_montserrat_24
#define FONT_XL         &lv_font_montserrat_30
#define FONT_XXL        &lv_font_montserrat_36
#define FONT_HUGE       &lv_font_montserrat_48

// =========================
// Gauge parameters
// =========================
#define GAUGE_TEMP_MIN      0
#define GAUGE_TEMP_MAX      120
#define GAUGE_ANGLE_RANGE   270
#define GAUGE_ROTATION      135

// =========================
// Chart parameters
// =========================
#define CHART_POINT_COUNT   120   // 120 points × 2s = 4 minutes of visible data
#define CHART_UPDATE_MS     2000  // Update every 2 seconds

// =========================
// Animation durations (ms)
// =========================
#define ANIM_BOOT_FADE      500
#define ANIM_SCREEN_TRANS    300
#define ANIM_VALUE_UPDATE    200

#endif // UI_THEME_H
