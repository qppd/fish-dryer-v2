// manual_operation_screen.cpp
// Fish Dryer V2 HMI - Step-by-step operating instructions
// Provides a visual quick-reference guide for operating the fish dryer machine.

#include "manual_operation_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "screen_manager.h"

// ============================================================
// Instruction data
// ============================================================
struct Instruction {
    const char* step;
    const char* icon;
    const char* title;
    const char* body;
};

static const Instruction INSTRUCTIONS[] = {
    {
        "01",
        LV_SYMBOL_DOWNLOAD,
        "Prepare the Fish",
        "Clean and gut the fish thoroughly. Pat dry with a cloth to remove\n"
        "excess surface moisture. Cut into uniform pieces (3-5 cm thick)\n"
        "for even drying. Arrange fish in a single layer on the drying tray."
    },
    {
        "02",
        LV_SYMBOL_DOWNLOAD,
        "Load the Tray",
        "Place the loaded tray onto the load cell platform inside the dryer.\n"
        "Close the dryer chamber door securely. Make sure the fish pieces\n"
        "are not touching the walls or heating elements."
    },
    {
        "03",
        LV_SYMBOL_REFRESH,
        "Tare the Scale",
        "Press SETTINGS from the menu, then tap TARE SCALE with the fish\n"
        "already loaded. Wait for the weight reading to stabilise at the\n"
        "initial fish weight before proceeding."
    },
    {
        "04",
        LV_SYMBOL_SETTINGS,
        "Set Target Temperature",
        "Navigate to CONTROL on the dashboard. Use the + / - buttons to set\n"
        "the drying temperature. Recommended range: 40-60 \xC2\xB0""C for most fish.\n"
        "Higher temperatures dry faster but may affect texture and flavor."
    },
    {
        "05",
        LV_SYMBOL_SETTINGS,
        "Set Water Loss Target",
        "In the CONTROL screen, set the target water loss percentage.\n"
        "A 40-50% water loss is typical for dried fish products.\n"
        "The dryer will stop automatically when this target is reached."
    },
    {
        "06",
        LV_SYMBOL_PLAY,
        "Start Drying",
        "Tap START DRYING on the dashboard or control screen. The system\n"
        "will automatically activate the heater, convection fan, and exhaust fan.\n"
        "The PID controller will maintain the target temperature."
    },
    {
        "07",
        LV_SYMBOL_EYE_OPEN,
        "Monitor Progress",
        "Watch the dashboard for real-time temperature, humidity, and weight.\n"
        "The water loss % updates as moisture is removed. The drying time\n"
        "counter shows elapsed time. Do not open the chamber during drying."
    },
    {
        "08",
        LV_SYMBOL_STOP,
        "Drying Complete",
        "The machine will automatically stop when the target water loss is\n"
        "reached. A COMPLETE status will be shown on the dashboard. The\n"
        "history screen will log this session's results automatically."
    },
    {
        "09",
        LV_SYMBOL_DOWNLOAD,
        "Unload and Store",
        "Wait 5 minutes after completion for the chamber to cool slightly.\n"
        "Remove the tray carefully. Transfer dried fish to airtight containers\n"
        "or vacuum-sealed bags. Store in a cool, dry place."
    },
    {
        "10",
        LV_SYMBOL_SETTINGS,
        "Cleaning & Maintenance",
        "After each use, wipe the inner chamber with a dry cloth.\n"
        "Check the exhaust filter weekly; clean if blocked. Re-calibrate the\n"
        "load cell every month using a known reference weight."
    },
};

static const uint8_t INSTRUCTION_COUNT = sizeof(INSTRUCTIONS) / sizeof(INSTRUCTIONS[0]);

// ============================================================
// Screen creation
// ============================================================
lv_obj_t* createManualOperationScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Top bar
    createTopBar(scr, "HOW TO USE", true);

    // Scrollable content area
    lv_obj_t* content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - TOP_BAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_hor(content, SIDE_PADDING, 0);
    lv_obj_set_style_pad_ver(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 12, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);

    // Subtitle
    lv_obj_t* subtitle = lv_label_create(content);
    lv_label_set_text(subtitle, "Follow these steps each time you use the Fish Dryer V2.");
    lv_obj_set_style_text_font(subtitle, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);

    // Instruction cards
    for (uint8_t i = 0; i < INSTRUCTION_COUNT; i++) {
        const Instruction& ins = INSTRUCTIONS[i];

        lv_obj_t* card = lv_obj_create(content);
        lv_obj_set_width(card, LV_PCT(100));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_bg_color(card, COLOR_BG_CARD, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, COLOR_BORDER, 0);
        lv_obj_set_style_pad_all(card, 14, 0);
        lv_obj_set_style_pad_row(card, 6, 0);
        lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(card, 14, 0);

        // Step badge (circle with number)
        lv_obj_t* badge = lv_obj_create(card);
        lv_obj_set_size(badge, 48, 48);
        lv_obj_set_style_radius(badge, 24, 0);
        lv_obj_set_style_bg_color(badge, COLOR_ACCENT, 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 0, 0);
        lv_obj_set_scrollbar_mode(badge, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t* stepNum = lv_label_create(badge);
        lv_label_set_text(stepNum, ins.step);
        lv_obj_set_style_text_font(stepNum, FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(stepNum, lv_color_white(), 0);
        lv_obj_center(stepNum);

        // Text column
        lv_obj_t* textCol = lv_obj_create(card);
        lv_obj_set_flex_grow(textCol, 1);
        lv_obj_set_height(textCol, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(textCol, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(textCol, 0, 0);
        lv_obj_set_style_pad_all(textCol, 0, 0);
        lv_obj_set_scrollbar_mode(textCol, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(textCol, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(textCol, 4, 0);

        // Title row: icon + title text
        lv_obj_t* titleRow = lv_obj_create(textCol);
        lv_obj_set_width(titleRow, LV_PCT(100));
        lv_obj_set_height(titleRow, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(titleRow, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(titleRow, 0, 0);
        lv_obj_set_style_pad_all(titleRow, 0, 0);
        lv_obj_set_scrollbar_mode(titleRow, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(titleRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(titleRow, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(titleRow, 8, 0);

        lv_obj_t* iconLbl = lv_label_create(titleRow);
        lv_label_set_text(iconLbl, ins.icon);
        lv_obj_set_style_text_font(iconLbl, FONT_NORMAL, 0);
        lv_obj_set_style_text_color(iconLbl, COLOR_ACCENT, 0);

        lv_obj_t* titleLbl = lv_label_create(titleRow);
        lv_label_set_text(titleLbl, ins.title);
        lv_obj_set_style_text_font(titleLbl, FONT_MEDIUM, 0);
        lv_obj_set_style_text_color(titleLbl, COLOR_TEXT_PRIMARY, 0);
        lv_label_set_long_mode(titleLbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_flex_grow(titleLbl, 1);

        // Body text
        lv_obj_t* bodyLbl = lv_label_create(textCol);
        lv_label_set_text(bodyLbl, ins.body);
        lv_obj_set_style_text_font(bodyLbl, FONT_NORMAL, 0);
        lv_obj_set_style_text_color(bodyLbl, COLOR_TEXT_SECONDARY, 0);
        lv_label_set_long_mode(bodyLbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(bodyLbl, LV_PCT(100));
    }

    // Bottom tip
    lv_obj_t* tip = lv_label_create(content);
    lv_label_set_text(tip,
        LV_SYMBOL_WARNING "  For service or calibration, go to SETTINGS from the dashboard.");
    lv_obj_set_style_text_font(tip, FONT_SMALL, 0);
    lv_obj_set_style_text_color(tip, COLOR_WARNING, 0);
    lv_obj_set_width(tip, LV_PCT(100));
    lv_label_set_long_mode(tip, LV_LABEL_LONG_WRAP);

    return scr;
}

// Static screen â€” no live data to update
void updateManualOperationScreen() {
    // Nothing to update; instructions are static content
}
