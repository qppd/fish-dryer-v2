// dashboard_screen.cpp
// Fish Dryer V2 HMI - Main dashboard with temperature gauge and live data

#include "dashboard_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "dryer_data.h"
#include "screen_manager.h"
#include "serial_protocol.h"
#include "alert_popup.h"

// Estimated Time of Completion target humidity (% RH considered "dried")
#define EDT_TARGET_HUMIDITY_PCT  35.0f
// Minimum elapsed minutes and humidity drop before EDT is meaningful
#define EDT_MIN_ELAPSED_MINS     3.0f
#define EDT_MIN_HUMIDITY_DROP    2.0f

// Widget references for updates
static lv_obj_t* tempMeter = NULL;
static lv_meter_indicator_t* tempNeedle = NULL;
static lv_obj_t* tempValueLabel = NULL;
static lv_obj_t* tempTargetLabel = NULL;
static lv_obj_t* humidityValueLabel = NULL;
static lv_obj_t* weightValueLabel = NULL;
static lv_obj_t* waterLossBar = NULL;
static lv_obj_t* waterLossLabel = NULL;
static lv_obj_t* heaterIndicator = NULL;
static lv_obj_t* fanIndicator = NULL;
static lv_obj_t* exhaustIndicator = NULL;
static lv_obj_t* stateLabel = NULL;
static lv_obj_t* connIcon = NULL;
static lv_obj_t* elapsedLabel = NULL;
static lv_obj_t* edtLabel = NULL;
static lv_obj_t* startBtn = NULL;
static lv_obj_t* stopBtn = NULL;

// EDT tracking state
static float        edt_startHumidity = 0.0f;
static unsigned long edt_startMs       = 0;
static DryerState   edt_lastState      = STATE_IDLE;

static void updateIndicator(lv_obj_t* dot, bool isOn);

// Optimistic UI state after START/STOP click for immediate feedback
static bool uiOptimisticActive = false;
static unsigned long uiOptimisticUntilMs = 0;
static bool optimisticHeaterOn = false;
static bool optimisticFanOn = false;
static bool optimisticExhaustOn = false;
static bool optimisticShowStart = true;
static bool optimisticShowStop = false;
static DryerState optimisticSystemState = STATE_IDLE;

static void applyStartStopVisibility(bool showStart, bool showStop) {
    if (lv_obj_is_valid(startBtn)) {
        if (showStart) lv_obj_clear_flag(startBtn, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(startBtn, LV_OBJ_FLAG_HIDDEN);
    }
    if (lv_obj_is_valid(stopBtn)) {
        if (showStop) lv_obj_clear_flag(stopBtn, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(stopBtn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void setOptimisticUiState(bool heaterOn, bool fanOn, bool exhaustOn,
                                 bool showStart, bool showStop, DryerState state) {
    uiOptimisticActive = true;
    uiOptimisticUntilMs = millis() + 15000;
    optimisticHeaterOn = heaterOn;
    optimisticFanOn = fanOn;
    optimisticExhaustOn = exhaustOn;
    optimisticShowStart = showStart;
    optimisticShowStop = showStop;
    optimisticSystemState = state;

    applyStartStopVisibility(showStart, showStop);
    updateIndicator(heaterIndicator, heaterOn);
    updateIndicator(fanIndicator, fanOn);
    updateIndicator(exhaustIndicator, exhaustOn);
}

// Navigation callbacks
static void navControlCb(lv_event_t* e) { (void)e; loadScreen(SCREEN_CONTROL); }
static void navAnalyticsCb(lv_event_t* e) { (void)e; loadScreen(SCREEN_ANALYTICS); }
static void navDiagnosticsCb(lv_event_t* e) { (void)e; loadScreen(SCREEN_DIAGNOSTICS); }

static void startBtnCb(lv_event_t* e) {
    (void)e;
    dryerData.systemState = STATE_DRYING;
    dryerData.heaterOn = true;
    dryerData.fanOn = true;
    dryerData.exhaustOn = false;
    dryerData.dryingStartMs = millis();
    setOptimisticUiState(true, true, false, false, true, STATE_DRYING);
    sendStartDrying();
}

static void stopBtnCb(lv_event_t* e) {
    (void)e;
    dryerData.systemState = STATE_IDLE;
    dryerData.heaterOn = false;
    dryerData.fanOn = false;
    dryerData.exhaustOn = true;
    setOptimisticUiState(false, false, true, true, false, STATE_IDLE);
    sendStopDrying();
}

// Create the temperature gauge (lv_meter)
static lv_obj_t* createTempGauge(lv_obj_t* parent) {
    lv_obj_t* meter = lv_meter_create(parent);
    lv_obj_set_size(meter, 260, 260);
    lv_obj_set_style_bg_color(meter, COLOR_GAUGE_BG, 0);
    lv_obj_set_style_border_width(meter, 0, 0);
    lv_obj_set_style_pad_all(meter, 12, 0);

    // Scale
    lv_meter_scale_t* scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 25, 2, 8, COLOR_TEXT_SECONDARY);
    lv_meter_set_scale_major_ticks(meter, scale, 4, 3, 14, COLOR_TEXT_PRIMARY, 12);
    lv_meter_set_scale_range(meter, scale, GAUGE_TEMP_MIN, GAUGE_TEMP_MAX, GAUGE_ANGLE_RANGE, GAUGE_ROTATION);

    // Color arcs: safe (green), hot (orange), danger (red)
    lv_meter_indicator_t* arcGreen = lv_meter_add_arc(meter, scale, 6, COLOR_GAUGE_SAFE, -2);
    lv_meter_set_indicator_start_value(meter, arcGreen, 0);
    lv_meter_set_indicator_end_value(meter, arcGreen, 70);

    lv_meter_indicator_t* arcOrange = lv_meter_add_arc(meter, scale, 6, COLOR_GAUGE_HOT, -2);
    lv_meter_set_indicator_start_value(meter, arcOrange, 70);
    lv_meter_set_indicator_end_value(meter, arcOrange, 90);

    lv_meter_indicator_t* arcRed = lv_meter_add_arc(meter, scale, 6, COLOR_GAUGE_DANGER, -2);
    lv_meter_set_indicator_start_value(meter, arcRed, 90);
    lv_meter_set_indicator_end_value(meter, arcRed, 120);

    // Needle
    tempNeedle = lv_meter_add_needle_line(meter, scale, 3, COLOR_ACCENT, -18);
    lv_meter_set_indicator_value(meter, tempNeedle, 0);

    return meter;
}

// Create an info card with label and value
static lv_obj_t* createInfoCard(lv_obj_t* parent, const char* icon, const char* title,
                                 lv_obj_t** valueLabelOut, lv_coord_t w, lv_coord_t h) {
    lv_obj_t* card = createCard(parent, w, h);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 4, 0);

    // Icon
    lv_obj_t* iconLabel = lv_label_create(card);
    lv_label_set_text(iconLabel, icon);
    lv_obj_set_style_text_font(iconLabel, FONT_LARGE, 0);
    lv_obj_set_style_text_color(iconLabel, COLOR_TEXT_SECONDARY, 0);

    // Value
    lv_obj_t* valueLabel = lv_label_create(card);
    lv_label_set_text(valueLabel, "--");
    lv_obj_set_style_text_font(valueLabel, FONT_XL, 0);
    lv_obj_set_style_text_color(valueLabel, COLOR_TEXT_PRIMARY, 0);
    *valueLabelOut = valueLabel;

    // Unit/title
    lv_obj_t* titleLabel = lv_label_create(card);
    lv_label_set_text(titleLabel, title);
    lv_obj_add_style(titleLabel, &style_text_label, 0);

    return card;
}

// Create a relay status indicator
static lv_obj_t* createRelayIndicator(lv_obj_t* parent, const char* name) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 90, 36);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 6, 0);

    // Status dot
    lv_obj_t* dot = lv_obj_create(cont);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_add_style(dot, &style_indicator_off, 0);
    lv_obj_set_scrollbar_mode(dot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(dot, 0, 0);

    // Label
    lv_obj_t* label = lv_label_create(cont);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label, COLOR_TEXT_SECONDARY, 0);

    return dot;  // Return the dot for updating
}

lv_obj_t* createDashboardScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // ==================== TOP BAR ====================
    lv_obj_t* topBar = createTopBar(scr, "SolAraw", false);

    // Connection status in top bar
    connIcon = lv_label_create(topBar);
    lv_label_set_text(connIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(connIcon, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(connIcon, COLOR_TEXT_SECONDARY, 0);

    // State badge in top bar
    stateLabel = lv_label_create(topBar);
    lv_label_set_text(stateLabel, "IDLE");
    lv_obj_set_style_text_font(stateLabel, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(stateLabel, COLOR_IDLE, 0);

    // ==================== CONTENT AREA ====================
    lv_obj_t* content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_WIDTH, CONTENT_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, SIDE_PADDING, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    // ---- Left: Temperature gauge ----
    lv_obj_t* gaugeContainer = lv_obj_create(content);
    lv_obj_set_size(gaugeContainer, 300, 320);
    lv_obj_align(gaugeContainer, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(gaugeContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gaugeContainer, 0, 0);
    lv_obj_set_style_pad_all(gaugeContainer, 0, 0);
    lv_obj_set_scrollbar_mode(gaugeContainer, LV_SCROLLBAR_MODE_OFF);

    tempMeter = createTempGauge(gaugeContainer);
    lv_obj_align(tempMeter, LV_ALIGN_TOP_MID, 0, 0);

    // Digital temperature readout below gauge
    tempValueLabel = lv_label_create(gaugeContainer);
    lv_label_set_text(tempValueLabel, "-- \u00B0C");
    lv_obj_set_style_text_font(tempValueLabel, FONT_XXL, 0);
    lv_obj_set_style_text_color(tempValueLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(tempValueLabel, LV_ALIGN_BOTTOM_MID, 0, -20);

    tempTargetLabel = lv_label_create(gaugeContainer);
    lv_label_set_text(tempTargetLabel, "Target: -- \u00B0C");
    lv_obj_set_style_text_font(tempTargetLabel, FONT_SMALL, 0);
    lv_obj_set_style_text_color(tempTargetLabel, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(tempTargetLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    // ---- Right column: Info cards + relay status ----
    lv_obj_t* rightCol = lv_obj_create(content);
    lv_obj_set_size(rightCol, 440, 320);
    lv_obj_align(rightCol, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(rightCol, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rightCol, 0, 0);
    lv_obj_set_style_pad_all(rightCol, 0, 0);
    lv_obj_set_scrollbar_mode(rightCol, LV_SCROLLBAR_MODE_OFF);

    // Info cards row
    lv_obj_t* cardsRow = lv_obj_create(rightCol);
    lv_obj_set_size(cardsRow, 440, 130);
    lv_obj_align(cardsRow, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(cardsRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cardsRow, 0, 0);
    lv_obj_set_style_pad_all(cardsRow, 0, 0);
    lv_obj_set_scrollbar_mode(cardsRow, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cardsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cardsRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    createInfoCard(cardsRow, LV_SYMBOL_TINT, "Humidity", &humidityValueLabel, 135, 120);
    createInfoCard(cardsRow, LV_SYMBOL_DOWNLOAD, "Weight", &weightValueLabel, 135, 120);

    // Water loss card (with bar)
    lv_obj_t* wlCard = createCard(cardsRow, 135, 120);
    lv_obj_set_flex_flow(wlCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wlCard, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wlCard, 4, 0);

    lv_obj_t* wlIcon = lv_label_create(wlCard);
    lv_label_set_text(wlIcon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(wlIcon, FONT_LARGE, 0);
    lv_obj_set_style_text_color(wlIcon, COLOR_TEXT_SECONDARY, 0);

    waterLossLabel = lv_label_create(wlCard);
    lv_label_set_text(waterLossLabel, "--%");
    lv_obj_set_style_text_font(waterLossLabel, FONT_XL, 0);
    lv_obj_set_style_text_color(waterLossLabel, COLOR_TEXT_PRIMARY, 0);

    lv_obj_t* wlTitle = lv_label_create(wlCard);
    lv_label_set_text(wlTitle, "Water Loss");
    lv_obj_add_style(wlTitle, &style_text_label, 0);

    // Relay status row
    lv_obj_t* relayRow = lv_obj_create(rightCol);
    lv_obj_set_size(relayRow, 440, 50);
    lv_obj_align(relayRow, LV_ALIGN_TOP_MID, 0, 138);
    lv_obj_set_style_bg_opa(relayRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(relayRow, 0, 0);
    lv_obj_set_style_pad_all(relayRow, 0, 0);
    lv_obj_set_scrollbar_mode(relayRow, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(relayRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(relayRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(relayRow, 15, 0);

    lv_obj_t* relayTitle = lv_label_create(relayRow);
    lv_label_set_text(relayTitle, "RELAYS:");
    lv_obj_set_style_text_font(relayTitle, FONT_SMALL, 0);
    lv_obj_set_style_text_color(relayTitle, COLOR_TEXT_SECONDARY, 0);

    heaterIndicator = createRelayIndicator(relayRow, "Heater");
    fanIndicator = createRelayIndicator(relayRow, "Fan");
    exhaustIndicator = createRelayIndicator(relayRow, "Exhaust");

    // Water loss progress bar
    waterLossBar = lv_bar_create(rightCol);
    lv_obj_set_size(waterLossBar, 420, 14);
    lv_obj_align(waterLossBar, LV_ALIGN_TOP_MID, 0, 195);
    lv_bar_set_range(waterLossBar, 0, 100);
    lv_bar_set_value(waterLossBar, 0, LV_ANIM_ON);
    lv_obj_set_style_bg_color(waterLossBar, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_color(waterLossBar, COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_set_style_radius(waterLossBar, 7, 0);
    lv_obj_set_style_radius(waterLossBar, 7, LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(waterLossBar, ANIM_VALUE_UPDATE, 0);

    // Elapsed time
    elapsedLabel = lv_label_create(rightCol);
    lv_label_set_text(elapsedLabel, "");
    lv_obj_set_style_text_font(elapsedLabel, FONT_SMALL, 0);
    lv_obj_set_style_text_color(elapsedLabel, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(elapsedLabel, LV_ALIGN_TOP_LEFT, 0, 218);

    // Estimated time of completion (EDT) label — right side of the same row
    edtLabel = lv_label_create(rightCol);
    lv_label_set_text(edtLabel, "");
    lv_obj_set_style_text_font(edtLabel, FONT_SMALL, 0);
    lv_obj_set_style_text_color(edtLabel, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(edtLabel, LV_ALIGN_TOP_RIGHT, 0, 218);

    // Navigation buttons row at bottom of right panel
    lv_obj_t* navRow = lv_obj_create(rightCol);
    lv_obj_set_size(navRow, 440, 80);
    lv_obj_align(navRow, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(navRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(navRow, 0, 0);
    lv_obj_set_style_pad_all(navRow, 0, 0);
    lv_obj_set_scrollbar_mode(navRow, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(navRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(navRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* ctrlBtn = createButton(navRow, LV_SYMBOL_SETTINGS " Control", 130, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(ctrlBtn, navControlCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* chartBtn = createButton(navRow, LV_SYMBOL_LIST " Analytics", 130, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(chartBtn, navAnalyticsCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* diagBtn = createButton(navRow, LV_SYMBOL_EYE_OPEN " Diag", 110, BTN_MIN_SIZE, &style_btn_nav);
    lv_obj_add_event_cb(diagBtn, navDiagnosticsCb, LV_EVENT_CLICKED, NULL);

    // ==================== BOTTOM BAR ====================
    lv_obj_t* bottomBar = lv_obj_create(scr);
    lv_obj_set_size(bottomBar, SCREEN_WIDTH, BOTTOM_BAR_HEIGHT);
    lv_obj_align(bottomBar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(bottomBar, COLOR_BG_TOP_BAR, 0);
    lv_obj_set_style_bg_opa(bottomBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(bottomBar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(bottomBar, 1, 0);
    lv_obj_set_style_border_color(bottomBar, COLOR_BORDER, 0);
    lv_obj_set_style_radius(bottomBar, 0, 0);
    lv_obj_set_style_pad_hor(bottomBar, SIDE_PADDING, 0);
    lv_obj_set_scrollbar_mode(bottomBar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(bottomBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottomBar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bottomBar, 20, 0);

    startBtn = createButton(bottomBar, LV_SYMBOL_PLAY " START", 160, 44, &style_btn_success);
    lv_obj_add_event_cb(startBtn, startBtnCb, LV_EVENT_CLICKED, NULL);

    stopBtn = createButton(bottomBar, LV_SYMBOL_STOP " STOP", 160, 44, &style_btn_danger);
    lv_obj_add_event_cb(stopBtn, stopBtnCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* setTempBtn = createButton(bottomBar, LV_SYMBOL_SETTINGS " SET TEMP", 180, 44, &style_btn_primary);
    lv_obj_add_event_cb(setTempBtn, navControlCb, LV_EVENT_CLICKED, NULL);

    return scr;
}

static void updateIndicator(lv_obj_t* dot, bool isOn) {
    if (!dot) return;
    if (isOn) {
        lv_obj_set_style_bg_color(dot, COLOR_SUCCESS, 0);
        lv_obj_set_style_shadow_color(dot, COLOR_SUCCESS, 0);
        lv_obj_set_style_shadow_width(dot, 8, 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_50, 0);
    } else {
        lv_obj_set_style_bg_color(dot, COLOR_BG_BUTTON, 0);
        lv_obj_set_style_shadow_width(dot, 0, 0);
    }
}

void updateDashboardScreen() {
    if (!tempMeter || !tempNeedle || !lv_obj_is_valid(tempMeter)) return;

    // Temperature gauge
    int tempVal = (int)dryerData.temperature;
    if (tempVal < GAUGE_TEMP_MIN) tempVal = GAUGE_TEMP_MIN;
    if (tempVal > GAUGE_TEMP_MAX) tempVal = GAUGE_TEMP_MAX;
    if (lv_obj_is_valid(tempMeter)) {
        lv_meter_set_indicator_value(tempMeter, tempNeedle, tempVal);
    }

    // Temperature value
    if (lv_obj_is_valid(tempValueLabel)) {
        { char _b[20]; snprintf(_b, sizeof(_b), "%.1f \xC2\xB0C", dryerData.temperature); lv_label_set_text(tempValueLabel, _b); }
    }
    if (lv_obj_is_valid(tempTargetLabel)) {
        { char _b[28]; snprintf(_b, sizeof(_b), "Target: %.0f \xC2\xB0C", dryerData.targetTemp); lv_label_set_text(tempTargetLabel, _b); }
    }

    // Humidity
    if (lv_obj_is_valid(humidityValueLabel)) {
        { char _b[12]; snprintf(_b, sizeof(_b), "%.0f%%", dryerData.humidity); lv_label_set_text(humidityValueLabel, _b); }
    }

    // Weight
    if (lv_obj_is_valid(weightValueLabel)) {
        { char _b[12]; snprintf(_b, sizeof(_b), "%.1f kg", dryerData.weight); lv_label_set_text(weightValueLabel, _b); }
    }

    // Water loss
    if (lv_obj_is_valid(waterLossLabel)) {
        { char _b[12]; snprintf(_b, sizeof(_b), "%.0f%%", dryerData.waterLoss); lv_label_set_text(waterLossLabel, _b); }
    }
    if (lv_obj_is_valid(waterLossBar)) {
        int wlVal = (int)dryerData.waterLoss;
        if (wlVal < 0) wlVal = 0;
        if (wlVal > 100) wlVal = 100;
        lv_bar_set_value(waterLossBar, wlVal, LV_ANIM_ON);

        // Color the water loss bar based on progress
        if (dryerData.waterLoss >= dryerData.targetWaterLoss) {
            lv_obj_set_style_bg_color(waterLossBar, COLOR_SUCCESS, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(waterLossBar, COLOR_IDLE, LV_PART_INDICATOR);
        }
    }

    if (uiOptimisticActive && millis() >= uiOptimisticUntilMs) {
        uiOptimisticActive = false;
    }
    DryerState effectiveState = uiOptimisticActive ? optimisticSystemState : dryerData.systemState;

    // Relay indicators
    if (uiOptimisticActive) {
        updateIndicator(heaterIndicator, optimisticHeaterOn);
        updateIndicator(fanIndicator, optimisticFanOn);
        updateIndicator(exhaustIndicator, optimisticExhaustOn);
    } else {
        updateIndicator(heaterIndicator, dryerData.heaterOn);
        updateIndicator(fanIndicator, dryerData.fanOn);
        updateIndicator(exhaustIndicator, dryerData.exhaustOn);
    }

    // System state
    if (lv_obj_is_valid(stateLabel)) {
        const char* stateText;
        switch (effectiveState) {
            case STATE_DRYING:   stateText = "DRYING"; break;
            case STATE_COMPLETE: stateText = "COMPLETE"; break;
            case STATE_ERROR:    stateText = "ERROR"; break;
            case STATE_PAUSED:   stateText = "PAUSED"; break;
            default:             stateText = "IDLE"; break;
        }
        lv_label_set_text(stateLabel, stateText);
        lv_obj_set_style_text_color(stateLabel, getStateColor(effectiveState), 0);
    }

    // ---- START/STOP Button Visibility based on state (or optimistic action) ----
    if (uiOptimisticActive) {
        applyStartStopVisibility(optimisticShowStart, optimisticShowStop);
    } else {
        switch (dryerData.systemState) {
            case STATE_IDLE:
                applyStartStopVisibility(true, false);
                break;
            case STATE_DRYING:
                applyStartStopVisibility(false, true);
                break;
            case STATE_COMPLETE:
            case STATE_ERROR:
                applyStartStopVisibility(false, false);
                break;
            case STATE_PAUSED:
                applyStartStopVisibility(true, true);
                break;
            default:
                applyStartStopVisibility(true, false);
                break;
        }
    }

    // Connection indicator
    if (lv_obj_is_valid(connIcon)) {
        lv_obj_set_style_text_color(connIcon, dryerData.connected ? COLOR_SUCCESS : COLOR_DANGER, 0);
    }

    // Elapsed time
    if (lv_obj_is_valid(elapsedLabel)) {
        if (effectiveState == STATE_DRYING && dryerData.dryingStartMs > 0) {
            unsigned long elapsed = millis() - dryerData.dryingStartMs;
            unsigned long secs = elapsed / 1000;
            unsigned long mins = secs / 60;
            unsigned long hrs = mins / 60;
            lv_label_set_text_fmt(elapsedLabel, "Elapsed: %luh %02lum", hrs, mins % 60);
        } else {
            lv_label_set_text(elapsedLabel, "");
        }
    }

    // ---- Estimated Drying Time (EDT) based on humidity decrease ----
    // Capture start-of-drying sample whenever a new drying session begins
    if (effectiveState == STATE_DRYING && edt_lastState != STATE_DRYING) {
        edt_startHumidity = dryerData.humidity;
        edt_startMs       = millis();
    }
    // Clear tracking when drying stops
    if (effectiveState != STATE_DRYING) {
        edt_startMs = 0;
    }
    edt_lastState = effectiveState;

    if (lv_obj_is_valid(edtLabel) && effectiveState == STATE_DRYING && edt_startMs > 0) {
        float elapsedMins  = (millis() - edt_startMs) / 60000.0f;
        float humidityDrop = edt_startHumidity - dryerData.humidity;

        if (elapsedMins >= EDT_MIN_ELAPSED_MINS && humidityDrop >= EDT_MIN_HUMIDITY_DROP) {
            float ratePerMin = humidityDrop / elapsedMins;          // %RH per minute
            float remaining  = dryerData.humidity - EDT_TARGET_HUMIDITY_PCT;

            if (remaining <= 0.0f) {
                lv_label_set_text(edtLabel, "EDT: Done");
            } else {
                unsigned long edtMins = (unsigned long)(remaining / ratePerMin);
                unsigned long edtHrs  = edtMins / 60;
                lv_label_set_text_fmt(edtLabel, "EDT: %lu:%02lu", edtHrs, edtMins % 60);
            }
        } else {
            lv_label_set_text(edtLabel, "EDT: --:--");
        }
    } else if (lv_obj_is_valid(edtLabel)) {
        lv_label_set_text(edtLabel, "");
    }
}
