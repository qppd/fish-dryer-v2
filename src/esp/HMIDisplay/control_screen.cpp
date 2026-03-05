// control_screen.cpp
// Fish Dryer V2 HMI - Manual control interface (industrial PLC style)

#include "control_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "dryer_data.h"
#include "screen_manager.h"
#include "serial_protocol.h"

// Widget references
static lv_obj_t* tempSetLabel = NULL;
static lv_obj_t* heaterSwitch = NULL;
static lv_obj_t* fanSwitch = NULL;
static lv_obj_t* exhaustSwitch = NULL;
static lv_obj_t* autoRadio = NULL;
static lv_obj_t* manualRadio = NULL;
static lv_obj_t* waterLossSlider = NULL;
static lv_obj_t* waterLossSliderLabel = NULL;
static lv_obj_t* currentTempLabel = NULL;
static lv_obj_t* startDryingBtn = NULL;

static float tempSetpoint = 60.0f;

// Temperature +/- callbacks
static void tempPlusCb(lv_event_t* e) {
    (void)e;
    tempSetpoint += 1.0f;
    if (tempSetpoint > 100.0f) tempSetpoint = 100.0f;
    lv_label_set_text_fmt(tempSetLabel, "%.0f \u00B0C", tempSetpoint);
    sendSetTemperature(tempSetpoint);
}

static void tempMinusCb(lv_event_t* e) {
    (void)e;
    tempSetpoint -= 1.0f;
    if (tempSetpoint < 30.0f) tempSetpoint = 30.0f;
    lv_label_set_text_fmt(tempSetLabel, "%.0f \u00B0C", tempSetpoint);
    sendSetTemperature(tempSetpoint);
}

// +5 / -5 for faster adjustment
static void tempPlus5Cb(lv_event_t* e) {
    (void)e;
    tempSetpoint += 5.0f;
    if (tempSetpoint > 100.0f) tempSetpoint = 100.0f;
    lv_label_set_text_fmt(tempSetLabel, "%.0f \u00B0C", tempSetpoint);
    sendSetTemperature(tempSetpoint);
}

static void tempMinus5Cb(lv_event_t* e) {
    (void)e;
    tempSetpoint -= 5.0f;
    if (tempSetpoint < 30.0f) tempSetpoint = 30.0f;
    lv_label_set_text_fmt(tempSetLabel, "%.0f \u00B0C", tempSetpoint);
    sendSetTemperature(tempSetpoint);
}

// Relay toggle callbacks
static void heaterToggleCb(lv_event_t* e) {
    bool on = lv_obj_has_state(heaterSwitch, LV_STATE_CHECKED);
    sendHeaterControl(on);
}

static void fanToggleCb(lv_event_t* e) {
    bool on = lv_obj_has_state(fanSwitch, LV_STATE_CHECKED);
    sendFanControl(on);
}

static void exhaustToggleCb(lv_event_t* e) {
    bool on = lv_obj_has_state(exhaustSwitch, LV_STATE_CHECKED);
    sendExhaustControl(on);
}

// Mode radio callbacks
static void autoModeCb(lv_event_t* e) {
    (void)e;
    lv_obj_add_state(autoRadio, LV_STATE_CHECKED);
    lv_obj_clear_state(manualRadio, LV_STATE_CHECKED);
    dryerData.dryingModeAuto = true;
    // Disable manual relay controls in auto mode
    lv_obj_add_state(heaterSwitch, LV_STATE_DISABLED);
    lv_obj_add_state(fanSwitch, LV_STATE_DISABLED);
    lv_obj_add_state(exhaustSwitch, LV_STATE_DISABLED);
}

static void manualModeCb(lv_event_t* e) {
    (void)e;
    lv_obj_clear_state(autoRadio, LV_STATE_CHECKED);
    lv_obj_add_state(manualRadio, LV_STATE_CHECKED);
    dryerData.dryingModeAuto = false;
    // Enable manual relay controls
    lv_obj_clear_state(heaterSwitch, LV_STATE_DISABLED);
    lv_obj_clear_state(fanSwitch, LV_STATE_DISABLED);
    lv_obj_clear_state(exhaustSwitch, LV_STATE_DISABLED);
}

// Water loss slider callback
static void waterLossSliderCb(lv_event_t* e) {
    int val = lv_slider_get_value(waterLossSlider);
    lv_label_set_text_fmt(waterLossSliderLabel, "%d %%", val);
    dryerData.targetWaterLoss = (float)val;
}

// Start drying callback
static void startDryingCb(lv_event_t* e) {
    (void)e;
    sendSetTemperature(tempSetpoint);
    sendStartDrying();
    loadScreen(SCREEN_DASHBOARD);
}

// Helper: create a labeled switch row
static lv_obj_t* createSwitchRow(lv_obj_t* parent, const char* label, lv_obj_t** switchOut,
                                  lv_event_cb_t cb) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);

    lv_obj_t* sw = lv_switch_create(row);
    lv_obj_set_size(sw, 60, 30);
    lv_obj_set_style_bg_color(sw, COLOR_BG_BUTTON, 0);
    lv_obj_set_style_bg_color(sw, COLOR_SUCCESS, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, NULL);
    *switchOut = sw;

    return row;
}

lv_obj_t* createControlScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Top bar
    createTopBar(scr, "CONTROL PANEL", true);

    // Initialize setpoint from current data
    tempSetpoint = dryerData.targetTemp;

    // Scrollable content area
    lv_obj_t* content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - TOP_BAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, SIDE_PADDING, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content, WIDGET_SPACING, 0);
    lv_obj_set_style_pad_column(content, WIDGET_SPACING, 0);

    // ==================== LEFT COLUMN ====================
    lv_obj_t* leftCol = createCard(content, 370, 390);
    lv_obj_set_flex_flow(leftCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(leftCol, 10, 0);

    // -- Temperature setpoint section --
    lv_obj_t* tempTitle = lv_label_create(leftCol);
    lv_label_set_text(tempTitle, "Set Temperature");
    lv_obj_add_style(tempTitle, &style_text_label, 0);

    lv_obj_t* tempRow = lv_obj_create(leftCol);
    lv_obj_set_size(tempRow, LV_PCT(100), 65);
    lv_obj_set_style_bg_opa(tempRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tempRow, 0, 0);
    lv_obj_set_style_pad_all(tempRow, 0, 0);
    lv_obj_set_scrollbar_mode(tempRow, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(tempRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tempRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tempRow, 8, 0);

    lv_obj_t* m5Btn = createButton(tempRow, "-5", BTN_MIN_SIZE, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(m5Btn, tempMinus5Cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* mBtn = createButton(tempRow, "-", BTN_MIN_SIZE, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(mBtn, tempMinusCb, LV_EVENT_CLICKED, NULL);

    // Temperature display card
    lv_obj_t* tempDisplay = lv_obj_create(tempRow);
    lv_obj_set_size(tempDisplay, 130, 55);
    lv_obj_set_style_bg_color(tempDisplay, lv_color_hex(0x141E30), 0);
    lv_obj_set_style_bg_opa(tempDisplay, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tempDisplay, 8, 0);
    lv_obj_set_style_border_width(tempDisplay, 1, 0);
    lv_obj_set_style_border_color(tempDisplay, COLOR_ACCENT, 0);
    lv_obj_set_scrollbar_mode(tempDisplay, LV_SCROLLBAR_MODE_OFF);

    tempSetLabel = lv_label_create(tempDisplay);
    lv_label_set_text_fmt(tempSetLabel, "%.0f \u00B0C", tempSetpoint);
    lv_obj_set_style_text_font(tempSetLabel, FONT_XL, 0);
    lv_obj_set_style_text_color(tempSetLabel, COLOR_ACCENT, 0);
    lv_obj_center(tempSetLabel);

    lv_obj_t* pBtn = createButton(tempRow, "+", BTN_MIN_SIZE, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(pBtn, tempPlusCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* p5Btn = createButton(tempRow, "+5", BTN_MIN_SIZE, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(p5Btn, tempPlus5Cb, LV_EVENT_CLICKED, NULL);

    // Current temperature display
    currentTempLabel = lv_label_create(leftCol);
    lv_label_set_text_fmt(currentTempLabel, "Current: %.1f \u00B0C", dryerData.temperature);
    lv_obj_set_style_text_font(currentTempLabel, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(currentTempLabel, COLOR_TEXT_SECONDARY, 0);

    // -- Relay controls --
    lv_obj_t* relaySep = lv_obj_create(leftCol);
    lv_obj_set_size(relaySep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(relaySep, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(relaySep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(relaySep, 0, 0);

    createSwitchRow(leftCol, LV_SYMBOL_CHARGE " Heater", &heaterSwitch, heaterToggleCb);
    createSwitchRow(leftCol, LV_SYMBOL_REFRESH " Convection Fan", &fanSwitch, fanToggleCb);
    createSwitchRow(leftCol, LV_SYMBOL_UPLOAD " Exhaust Fan", &exhaustSwitch, exhaustToggleCb);

    // ==================== RIGHT COLUMN ====================
    lv_obj_t* rightCol = createCard(content, 370, 390);
    lv_obj_set_flex_flow(rightCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rightCol, 12, 0);

    // -- Drying mode --
    lv_obj_t* modeTitle = lv_label_create(rightCol);
    lv_label_set_text(modeTitle, "Drying Mode");
    lv_obj_add_style(modeTitle, &style_text_label, 0);

    lv_obj_t* modeRow = lv_obj_create(rightCol);
    lv_obj_set_size(modeRow, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(modeRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(modeRow, 0, 0);
    lv_obj_set_style_pad_all(modeRow, 0, 0);
    lv_obj_set_scrollbar_mode(modeRow, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(modeRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(modeRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(modeRow, 20, 0);

    // Auto radio
    autoRadio = lv_checkbox_create(modeRow);
    lv_checkbox_set_text(autoRadio, " Auto");
    lv_obj_set_style_text_color(autoRadio, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(autoRadio, FONT_MEDIUM, 0);
    lv_obj_add_state(autoRadio, LV_STATE_CHECKED);
    lv_obj_add_event_cb(autoRadio, autoModeCb, LV_EVENT_CLICKED, NULL);

    // Manual radio
    manualRadio = lv_checkbox_create(modeRow);
    lv_checkbox_set_text(manualRadio, " Manual");
    lv_obj_set_style_text_color(manualRadio, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(manualRadio, FONT_MEDIUM, 0);
    lv_obj_add_event_cb(manualRadio, manualModeCb, LV_EVENT_CLICKED, NULL);

    // -- Target water loss --
    lv_obj_t* wlSep = lv_obj_create(rightCol);
    lv_obj_set_size(wlSep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(wlSep, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(wlSep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wlSep, 0, 0);

    lv_obj_t* wlTitle = lv_label_create(rightCol);
    lv_label_set_text(wlTitle, "Target Water Loss");
    lv_obj_add_style(wlTitle, &style_text_label, 0);

    waterLossSlider = lv_slider_create(rightCol);
    lv_obj_set_width(waterLossSlider, LV_PCT(90));
    lv_slider_set_range(waterLossSlider, 10, 95);
    lv_slider_set_value(waterLossSlider, (int)dryerData.targetWaterLoss, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(waterLossSlider, COLOR_BG_BUTTON, 0);
    lv_obj_set_style_bg_color(waterLossSlider, COLOR_IDLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(waterLossSlider, COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_set_style_pad_all(waterLossSlider, 6, LV_PART_KNOB);
    lv_obj_add_event_cb(waterLossSlider, waterLossSliderCb, LV_EVENT_VALUE_CHANGED, NULL);

    waterLossSliderLabel = lv_label_create(rightCol);
    lv_label_set_text_fmt(waterLossSliderLabel, "%d %%", (int)dryerData.targetWaterLoss);
    lv_obj_set_style_text_font(waterLossSliderLabel, FONT_XL, 0);
    lv_obj_set_style_text_color(waterLossSliderLabel, COLOR_TEXT_PRIMARY, 0);

    // -- Start drying button --
    lv_obj_t* btnSep = lv_obj_create(rightCol);
    lv_obj_set_size(btnSep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(btnSep, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(btnSep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btnSep, 0, 0);

    // Spacer
    lv_obj_t* spacer = lv_obj_create(rightCol);
    lv_obj_set_size(spacer, 1, 10);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    startDryingBtn = createButton(rightCol, LV_SYMBOL_PLAY "  START DRYING", LV_PCT(100), 55, &style_btn_success);
    lv_obj_add_event_cb(startDryingBtn, startDryingCb, LV_EVENT_CLICKED, NULL);

    // Set initial mode state (Auto = relay switches disabled)
    if (dryerData.dryingModeAuto) {
        lv_obj_add_state(heaterSwitch, LV_STATE_DISABLED);
        lv_obj_add_state(fanSwitch, LV_STATE_DISABLED);
        lv_obj_add_state(exhaustSwitch, LV_STATE_DISABLED);
    }

    return scr;
}

void updateControlScreen() {
    if (!currentTempLabel) return;

    lv_label_set_text_fmt(currentTempLabel, "Current: %.1f \u00B0C", dryerData.temperature);

    // Sync switch states with actual relay states
    if (dryerData.heaterOn) lv_obj_add_state(heaterSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(heaterSwitch, LV_STATE_CHECKED);

    if (dryerData.fanOn) lv_obj_add_state(fanSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(fanSwitch, LV_STATE_CHECKED);

    if (dryerData.exhaustOn) lv_obj_add_state(exhaustSwitch, LV_STATE_CHECKED);
    else lv_obj_clear_state(exhaustSwitch, LV_STATE_CHECKED);
}
