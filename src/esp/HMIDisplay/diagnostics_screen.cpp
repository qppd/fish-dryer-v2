// diagnostics_screen.cpp
// Fish Dryer V2 HMI - System diagnostics and maintenance screen

#include "diagnostics_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "dryer_data.h"
#include "screen_manager.h"
#include "serial_protocol.h"

// Widget references for updates
static lv_obj_t* tempSensorDot = NULL;
static lv_obj_t* tempSensorText = NULL;
static lv_obj_t* loadCellDot = NULL;
static lv_obj_t* loadCellText = NULL;
static lv_obj_t* fanCurrentLabel = NULL;
static lv_obj_t* heaterPowerLabel = NULL;
static lv_obj_t* batteryLabel = NULL;
static lv_obj_t* i2cLabel = NULL;
static lv_obj_t* connStatusLabel = NULL;
static lv_obj_t* lastUpdateLabel = NULL;
static lv_obj_t* uptimeLabel = NULL;
static lv_obj_t* freeHeapLabel = NULL;

// Run sensor test callback
static void runTestCb(lv_event_t* e) {
    (void)e;
    // Send test commands to dryer controller
    sendStatusRequest();
    DRYER_UART.println("SHT31:READ");
    DRYER_UART.println("LOADCELL:READ");
}

// Helper: create a diagnostic row with status dot
static void createDiagRow(lv_obj_t* parent, const char* name, const char* icon,
                           lv_obj_t** dotOut, lv_obj_t** textOut) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 44);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);

    // Status dot
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 14, 14);
    lv_obj_add_style(dot, &style_indicator_off, 0);
    lv_obj_set_scrollbar_mode(dot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(dot, 0, 0);
    if (dotOut) *dotOut = dot;

    // Icon + name
    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text_fmt(lbl, "%s %s", icon, name);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_flex_grow(lbl, 1);

    // Status text
    lv_obj_t* statusText = lv_label_create(row);
    lv_label_set_text(statusText, "---");
    lv_obj_set_style_text_font(statusText, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(statusText, COLOR_TEXT_SECONDARY, 0);
    if (textOut) *textOut = statusText;
}

// Helper: create an info row with label and value
static lv_obj_t* createInfoRow(lv_obj_t* parent, const char* label, const char* initialValue) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 36);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_SECONDARY, 0);

    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, initialValue);
    lv_obj_set_style_text_font(val, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(val, COLOR_TEXT_PRIMARY, 0);

    return val;
}

lv_obj_t* createDiagnosticsScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Top bar
    createTopBar(scr, "SYSTEM DIAGNOSTICS", true);

    // Content - two columns
    lv_obj_t* content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - TOP_BAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, SIDE_PADDING, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(content, WIDGET_SPACING, 0);

    // ============ Left column: Sensor Status ============
    lv_obj_t* leftCard = createCard(content, 370, 400);
    lv_obj_set_flex_flow(leftCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(leftCard, 6, 0);

    lv_obj_t* sensorTitle = lv_label_create(leftCard);
    lv_label_set_text(sensorTitle, "SENSOR STATUS");
    lv_obj_add_style(sensorTitle, &style_text_label, 0);
    lv_obj_set_style_text_letter_space(sensorTitle, 2, 0);

    // Sensor rows
    createDiagRow(leftCard, "Temperature Sensor", LV_SYMBOL_CHARGE, &tempSensorDot, &tempSensorText);
    createDiagRow(leftCard, "Load Cell", LV_SYMBOL_DOWNLOAD, &loadCellDot, &loadCellText);

    // Separator
    lv_obj_t* sep1 = lv_obj_create(leftCard);
    lv_obj_set_size(sep1, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep1, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep1, 0, 0);

    // Power readings
    lv_obj_t* powerTitle = lv_label_create(leftCard);
    lv_label_set_text(powerTitle, "POWER READINGS");
    lv_obj_add_style(powerTitle, &style_text_label, 0);
    lv_obj_set_style_text_letter_space(powerTitle, 2, 0);

    fanCurrentLabel = createInfoRow(leftCard, "Fan Current", "--- A");
    heaterPowerLabel = createInfoRow(leftCard, "Heater Power", "--- W");
    batteryLabel = createInfoRow(leftCard, "Battery Voltage", "--- V");

    // Separator
    lv_obj_t* sep2 = lv_obj_create(leftCard);
    lv_obj_set_size(sep2, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep2, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep2, 0, 0);

    // I2C device status
    lv_obj_t* i2cTitle = lv_label_create(leftCard);
    lv_label_set_text(i2cTitle, "I2C DEVICES");
    lv_obj_add_style(i2cTitle, &style_text_label, 0);
    lv_obj_set_style_text_letter_space(i2cTitle, 2, 0);

    i2cLabel = createInfoRow(leftCard, "SHT31 Sensor", "not detected");

    // ============ Right column: System Info ============
    lv_obj_t* rightCard = createCard(content, 370, 400);
    lv_obj_set_flex_flow(rightCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rightCard, 6, 0);

    lv_obj_t* sysTitle = lv_label_create(rightCard);
    lv_label_set_text(sysTitle, "SYSTEM INFO");
    lv_obj_add_style(sysTitle, &style_text_label, 0);
    lv_obj_set_style_text_letter_space(sysTitle, 2, 0);

    connStatusLabel = createInfoRow(rightCard, "Controller Link", "Disconnected");
    lastUpdateLabel = createInfoRow(rightCard, "Last Update", "Never");
    uptimeLabel = createInfoRow(rightCard, "HMI Uptime", "0:00:00");
    freeHeapLabel = createInfoRow(rightCard, "Free Heap", "--- KB");

    // Firmware info (static)
    createInfoRow(rightCard, "HMI Version", "v2.0.0");
    createInfoRow(rightCard, "Display", "800x480 RGB");
    lv_obj_t* lvglVersionLabel = createInfoRow(rightCard, "LVGL Version", "");
    lv_label_set_text_fmt(lvglVersionLabel, "v%d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);

    // Separator
    lv_obj_t* sep3 = lv_obj_create(rightCard);
    lv_obj_set_size(sep3, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep3, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep3, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep3, 0, 0);

    // Spacer
    lv_obj_t* spacer = lv_obj_create(rightCard);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_scrollbar_mode(spacer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_width(spacer, 1);

    // Run test button
    lv_obj_t* testBtn = createButton(rightCard, LV_SYMBOL_REFRESH "  RUN SENSOR TEST",
                                      LV_PCT(100), 55, &style_btn_primary);
    lv_obj_add_event_cb(testBtn, runTestCb, LV_EVENT_CLICKED, NULL);

    return scr;
}

static void updateSensorDot(lv_obj_t* dot, SensorStatus status) {
    if (!dot) return;
    lv_color_t color;
    switch (status) {
        case SENSOR_OK:        color = COLOR_SUCCESS; break;
        case SENSOR_WARNING:   color = COLOR_WARNING; break;
        case SENSOR_ERROR:
        case SENSOR_NOT_FOUND: color = COLOR_DANGER; break;
        default:               color = COLOR_BG_BUTTON; break;
    }
    lv_obj_set_style_bg_color(dot, color, 0);
    if (status == SENSOR_OK) {
        lv_obj_set_style_shadow_color(dot, color, 0);
        lv_obj_set_style_shadow_width(dot, 8, 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_50, 0);
    } else {
        lv_obj_set_style_shadow_width(dot, 0, 0);
    }
}

static const char* sensorStatusText(SensorStatus status) {
    switch (status) {
        case SENSOR_OK:        return "OK";
        case SENSOR_WARNING:   return "WARNING";
        case SENSOR_ERROR:     return "ERROR";
        case SENSOR_NOT_FOUND: return "NOT FOUND";
        default:               return "UNKNOWN";
    }
}

void updateDiagnosticsScreen() {
    if (!tempSensorDot) return;

    // Sensor statuses
    updateSensorDot(tempSensorDot, dryerData.tempSensorStatus);
    lv_label_set_text(tempSensorText, sensorStatusText(dryerData.tempSensorStatus));

    updateSensorDot(loadCellDot, dryerData.loadCellStatus);
    lv_label_set_text(loadCellText, sensorStatusText(dryerData.loadCellStatus));

    // Power readings
    lv_label_set_text_fmt(fanCurrentLabel, "%.2f A", dryerData.fanCurrent);
    lv_label_set_text_fmt(heaterPowerLabel, "%.0f W", dryerData.heaterPower);
    lv_label_set_text_fmt(batteryLabel, "%.1f V", dryerData.batteryVoltage);

    // I2C
    lv_label_set_text(i2cLabel, dryerData.sht31Detected ? "detected" : "not detected");
    lv_obj_set_style_text_color(i2cLabel,
        dryerData.sht31Detected ? COLOR_SUCCESS : COLOR_DANGER, 0);

    // Connection status
    lv_label_set_text(connStatusLabel, dryerData.connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(connStatusLabel,
        dryerData.connected ? COLOR_SUCCESS : COLOR_DANGER, 0);

    // Last update
    if (dryerData.lastUpdateMs > 0) {
        unsigned long ago = (millis() - dryerData.lastUpdateMs) / 1000;
        if (ago < 2) {
            lv_label_set_text(lastUpdateLabel, "Just now");
        } else {
            lv_label_set_text_fmt(lastUpdateLabel, "%lu sec ago", ago);
        }
    }

    // Uptime
    unsigned long up = millis() / 1000;
    unsigned long h = up / 3600;
    unsigned long m = (up % 3600) / 60;
    unsigned long s = up % 60;
    lv_label_set_text_fmt(uptimeLabel, "%lu:%02lu:%02lu", h, m, s);

    // Free heap
    lv_label_set_text_fmt(freeHeapLabel, "%lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
}
