// alert_popup.cpp
// Fish Dryer V2 HMI - Alert and notification popup implementation

#include "alert_popup.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "dryer_data.h"

static lv_obj_t* activeAlert = NULL;
static bool alertShown = false;
static bool dryingCompleteShown = false;
static DryerState lastKnownState = STATE_IDLE;

// Dismiss callback
static void alertDismissCb(lv_event_t* e) {
    (void)e;
    dismissAlert();
}

void alertInit() {
    activeAlert = NULL;
    alertShown = false;
    dryingCompleteShown = false;
    lastKnownState = STATE_IDLE;
}

void showAlert(const char* title, const char* message, AlertType type) {
    // Dismiss any existing alert first
    dismissAlert();

    // Determine color
    lv_color_t color;
    const char* icon;
    switch (type) {
        case ALERT_WARNING:
            color = COLOR_WARNING;
            icon = LV_SYMBOL_WARNING;
            break;
        case ALERT_ERROR:
            color = COLOR_DANGER;
            icon = LV_SYMBOL_CLOSE;
            break;
        case ALERT_SUCCESS:
            color = COLOR_SUCCESS;
            icon = LV_SYMBOL_OK;
            break;
        default:
            color = COLOR_IDLE;
            icon = LV_SYMBOL_BELL;
            break;
    }

    // Create semi-transparent overlay on current screen
    lv_obj_t* overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_scrollbar_mode(overlay, LV_SCROLLBAR_MODE_OFF);
    activeAlert = overlay;

    // Alert box
    lv_obj_t* box = lv_obj_create(overlay);
    lv_obj_set_size(box, 400, 240);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 16, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, color, 0);
    lv_obj_set_style_pad_all(box, 20, 0);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 12, 0);

    // Shadow
    lv_obj_set_style_shadow_color(box, color, 0);
    lv_obj_set_style_shadow_width(box, 30, 0);
    lv_obj_set_style_shadow_opa(box, LV_OPA_30, 0);

    // Icon
    lv_obj_t* iconLbl = lv_label_create(box);
    lv_label_set_text(iconLbl, icon);
    lv_obj_set_style_text_font(iconLbl, FONT_XXL, 0);
    lv_obj_set_style_text_color(iconLbl, color, 0);

    // Title
    lv_obj_t* titleLbl = lv_label_create(box);
    lv_label_set_text(titleLbl, title);
    lv_obj_set_style_text_font(titleLbl, FONT_LARGE, 0);
    lv_obj_set_style_text_color(titleLbl, COLOR_TEXT_PRIMARY, 0);

    // Message
    lv_obj_t* msgLbl = lv_label_create(box);
    lv_label_set_text(msgLbl, message);
    lv_obj_set_style_text_font(msgLbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(msgLbl, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(msgLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(msgLbl, 340);

    // Acknowledge button
    lv_obj_t* ackBtn = lv_btn_create(box);
    lv_obj_set_size(ackBtn, 180, 44);
    lv_obj_set_style_bg_color(ackBtn, color, 0);
    lv_obj_set_style_radius(ackBtn, 8, 0);
    lv_obj_add_event_cb(ackBtn, alertDismissCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* ackLabel = lv_label_create(ackBtn);
    lv_label_set_text(ackLabel, "ACKNOWLEDGE");
    lv_obj_set_style_text_font(ackLabel, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ackLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(ackLabel);

    alertShown = true;
}

void showDryingComplete(float waterLoss, unsigned long elapsedMs) {
    dismissAlert();

    unsigned long secs = elapsedMs / 1000;
    unsigned long mins = secs / 60;
    unsigned long hrs = mins / 60;

    // Create overlay
    lv_obj_t* overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_scrollbar_mode(overlay, LV_SCROLLBAR_MODE_OFF);
    activeAlert = overlay;

    // Completion box
    lv_obj_t* box = lv_obj_create(overlay);
    lv_obj_set_size(box, 450, 300);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 16, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, COLOR_SUCCESS, 0);
    lv_obj_set_style_pad_all(box, 24, 0);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 10, 0);

    // Shadow
    lv_obj_set_style_shadow_color(box, COLOR_SUCCESS, 0);
    lv_obj_set_style_shadow_width(box, 30, 0);
    lv_obj_set_style_shadow_opa(box, LV_OPA_30, 0);

    // Icon
    lv_obj_t* iconLbl = lv_label_create(box);
    lv_label_set_text(iconLbl, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(iconLbl, FONT_HUGE, 0);
    lv_obj_set_style_text_color(iconLbl, COLOR_SUCCESS, 0);

    // Title
    lv_obj_t* titleLbl = lv_label_create(box);
    lv_label_set_text(titleLbl, "DRYING COMPLETE");
    lv_obj_set_style_text_font(titleLbl, FONT_LARGE, 0);
    lv_obj_set_style_text_color(titleLbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_letter_space(titleLbl, 2, 0);

    // Stats
    lv_obj_t* wlLbl = lv_label_create(box);
    { char _b[24]; snprintf(_b, sizeof(_b), "Water Loss: %.0f%%", waterLoss); lv_label_set_text(wlLbl, _b); }
    lv_obj_set_style_text_font(wlLbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(wlLbl, COLOR_TEXT_PRIMARY, 0);

    lv_obj_t* timeLbl = lv_label_create(box);
    lv_label_set_text_fmt(timeLbl, "Time: %luh %02lum", hrs, mins % 60);
    lv_obj_set_style_text_font(timeLbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(timeLbl, COLOR_TEXT_PRIMARY, 0);

    // Instruction
    lv_obj_t* instrLbl = lv_label_create(box);
    lv_label_set_text(instrLbl, "Remove product from dryer");
    lv_obj_set_style_text_font(instrLbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(instrLbl, COLOR_TEXT_SECONDARY, 0);

    // OK button
    lv_obj_t* okBtn = lv_btn_create(box);
    lv_obj_set_size(okBtn, 180, 44);
    lv_obj_set_style_bg_color(okBtn, COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(okBtn, 8, 0);
    lv_obj_add_event_cb(okBtn, alertDismissCb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* okLabel = lv_label_create(okBtn);
    lv_label_set_text(okLabel, "OK");
    lv_obj_set_style_text_font(okLabel, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(okLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(okLabel);

    dryingCompleteShown = true;
}

void dismissAlert() {
    if (activeAlert) {
        lv_obj_del(activeAlert);
        activeAlert = NULL;
        alertShown = false;
    }
}

void checkAlerts() {
    // Temperature too high alert
    if (dryerData.temperature > 95.0f && !alertShown) {
        showAlert("WARNING", "Temperature too high!\nCheck heating element.", ALERT_WARNING);
    }

    // Temperature critical
    if (dryerData.temperature > 110.0f && !alertShown) {
        showAlert("EMERGENCY", "Critical temperature!\nSystem shutting down.", ALERT_ERROR);
    }

    // Connection lost
    if (!dryerData.connected && dryerData.lastUpdateMs > 0 && !alertShown) {
        // Only alert after we've had a connection that was lost
        static unsigned long lastConnAlert = 0;
        if (millis() - lastConnAlert > 30000) {  // Don't spam - once per 30s
            showAlert("CONNECTION LOST", "Communication with dryer\ncontroller interrupted.", ALERT_ERROR);
            lastConnAlert = millis();
        }
    }

    // Drying complete detection
    if (dryerData.systemState == STATE_COMPLETE && lastKnownState == STATE_DRYING && !dryingCompleteShown) {
        unsigned long elapsed = 0;
        if (dryerData.dryingStartMs > 0) {
            elapsed = millis() - dryerData.dryingStartMs;
        }
        showDryingComplete(dryerData.waterLoss, elapsed);
    }

    lastKnownState = dryerData.systemState;
}
