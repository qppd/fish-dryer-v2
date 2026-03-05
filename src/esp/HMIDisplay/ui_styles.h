// ui_styles.h
// Fish Dryer V2 HMI - Reusable LVGL styles

#ifndef UI_STYLES_H
#define UI_STYLES_H

#include <lvgl.h>

// Style objects
extern lv_style_t style_screen_bg;
extern lv_style_t style_top_bar;
extern lv_style_t style_card;
extern lv_style_t style_card_dark;
extern lv_style_t style_btn_primary;
extern lv_style_t style_btn_danger;
extern lv_style_t style_btn_success;
extern lv_style_t style_btn_nav;
extern lv_style_t style_text_title;
extern lv_style_t style_text_value;
extern lv_style_t style_text_label;
extern lv_style_t style_text_small;
extern lv_style_t style_indicator_on;
extern lv_style_t style_indicator_off;

// Initialize all styles
void initStyles();

// Helper: create a styled card container
lv_obj_t* createCard(lv_obj_t* parent, lv_coord_t w, lv_coord_t h);

// Helper: create a styled button
lv_obj_t* createButton(lv_obj_t* parent, const char* text, lv_coord_t w, lv_coord_t h, lv_style_t* style);

// Helper: create a top navigation bar
lv_obj_t* createTopBar(lv_obj_t* parent, const char* title, bool showBack);

// Helper: get state color
lv_color_t getStateColor(int stateEnum);

#endif // UI_STYLES_H
