// ui_styles.cpp - Minimal styles implementation for standalone bootscreen

#include "ui_styles.h"
#include "ui_theme.h"

lv_style_t style_screen_bg;

void initStyles() {
    // Screen background style
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, COLOR_BG);
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen_bg, 0);
    lv_style_set_pad_all(&style_screen_bg, 0);
}
