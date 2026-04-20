// analytics_screen.cpp
// Fish Dryer V2 HMI - Real-time drying analytics with charts

#include "analytics_screen.h"
#include "ui_theme.h"
#include "ui_styles.h"
#include "dryer_data.h"
#include "screen_manager.h"

// Chart widget references
static lv_obj_t* tempChart = NULL;
static lv_obj_t* humChart = NULL;
static lv_obj_t* weightChart = NULL;
static lv_chart_series_t* tempSeries = NULL;
static lv_chart_series_t* humSeries = NULL;
static lv_chart_series_t* weightSeries = NULL;

// Current value labels
static lv_obj_t* tempCurLabel = NULL;
static lv_obj_t* humCurLabel = NULL;
static lv_obj_t* weightCurLabel = NULL;

// Tab view
static lv_obj_t* tabview = NULL;

// Create a single chart with styling
static lv_obj_t* createStyledChart(lv_obj_t* parent, lv_chart_series_t** seriesOut,
                                    lv_color_t color, lv_coord_t minY, lv_coord_t maxY) {
    lv_obj_t* chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 710, 280);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINT_COUNT);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, minY, maxY);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 5, 8);

    // Styling
    lv_obj_set_style_bg_color(chart, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chart, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_radius(chart, 8, 0);
    lv_obj_set_style_pad_all(chart, 10, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x1E293B), 0); // Grid lines
    lv_obj_set_style_line_opa(chart, LV_OPA_50, 0);

    // Series
    *seriesOut = lv_chart_add_series(chart, color, LV_CHART_AXIS_PRIMARY_Y);

    // Line style
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // Hide data points

    // Initialize series with zeros
    for (int i = 0; i < CHART_POINT_COUNT; i++) {
        lv_chart_set_next_value(chart, *seriesOut, 0);
    }

    return chart;
}

lv_obj_t* createAnalyticsScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Top bar
    createTopBar(scr, "DRYING ANALYTICS", true);

    // Tab view for charts
    tabview = lv_tabview_create(scr, LV_DIR_TOP, 40);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - TOP_BAR_HEIGHT);
    lv_obj_align(tabview, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_HEIGHT);

    // Style the tab view
    lv_obj_set_style_bg_color(tabview, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(tabview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tabview, 0, 0);

    // Style tab buttons
    lv_obj_t* tabBtns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tabBtns, COLOR_BG_TOP_BAR, 0);
    lv_obj_set_style_bg_opa(tabBtns, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(tabBtns, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_color(tabBtns, COLOR_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(tabBtns, COLOR_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tabBtns, FONT_NORMAL, 0);

    // ============ Temperature Tab ============
    lv_obj_t* tempTab = lv_tabview_add_tab(tabview, LV_SYMBOL_CHARGE " Temperature");
    lv_obj_set_style_bg_color(tempTab, COLOR_BG, 0);
    lv_obj_set_style_pad_all(tempTab, 8, 0);

    // Current value header
    lv_obj_t* tempHeader = lv_obj_create(tempTab);
    lv_obj_set_size(tempHeader, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(tempHeader, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tempHeader, 0, 0);
    lv_obj_set_style_pad_all(tempHeader, 0, 0);
    lv_obj_set_scrollbar_mode(tempHeader, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* tempHeaderTitle = lv_label_create(tempHeader);
    lv_label_set_text(tempHeaderTitle, "Temperature (\u00B0C)");
    lv_obj_set_style_text_font(tempHeaderTitle, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(tempHeaderTitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(tempHeaderTitle, LV_ALIGN_LEFT_MID, 0, 0);

    tempCurLabel = lv_label_create(tempHeader);
    lv_label_set_text(tempCurLabel, "-- \u00B0C");
    lv_obj_set_style_text_font(tempCurLabel, FONT_LARGE, 0);
    lv_obj_set_style_text_color(tempCurLabel, COLOR_CHART_TEMP, 0);
    lv_obj_align(tempCurLabel, LV_ALIGN_RIGHT_MID, 0, 0);

    tempChart = createStyledChart(tempTab, &tempSeries, COLOR_CHART_TEMP, 0, 120);

    // ============ Humidity Tab ============
    lv_obj_t* humTab = lv_tabview_add_tab(tabview, LV_SYMBOL_TINT " Humidity");
    lv_obj_set_style_bg_color(humTab, COLOR_BG, 0);
    lv_obj_set_style_pad_all(humTab, 8, 0);

    lv_obj_t* humHeader = lv_obj_create(humTab);
    lv_obj_set_size(humHeader, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(humHeader, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(humHeader, 0, 0);
    lv_obj_set_style_pad_all(humHeader, 0, 0);
    lv_obj_set_scrollbar_mode(humHeader, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* humHeaderTitle = lv_label_create(humHeader);
    lv_label_set_text(humHeaderTitle, "Relative Humidity (%RH)");
    lv_obj_set_style_text_font(humHeaderTitle, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(humHeaderTitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(humHeaderTitle, LV_ALIGN_LEFT_MID, 0, 0);

    humCurLabel = lv_label_create(humHeader);
    lv_label_set_text(humCurLabel, "-- %RH");
    lv_obj_set_style_text_font(humCurLabel, FONT_LARGE, 0);
    lv_obj_set_style_text_color(humCurLabel, COLOR_CHART_HUM, 0);
    lv_obj_align(humCurLabel, LV_ALIGN_RIGHT_MID, 0, 0);

    humChart = createStyledChart(humTab, &humSeries, COLOR_CHART_HUM, 0, 100);

    // ============ Weight Tab ============
    lv_obj_t* weightTab = lv_tabview_add_tab(tabview, LV_SYMBOL_DOWNLOAD " Weight");
    lv_obj_set_style_bg_color(weightTab, COLOR_BG, 0);
    lv_obj_set_style_pad_all(weightTab, 8, 0);

    lv_obj_t* weightHeader = lv_obj_create(weightTab);
    lv_obj_set_size(weightHeader, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(weightHeader, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(weightHeader, 0, 0);
    lv_obj_set_style_pad_all(weightHeader, 0, 0);
    lv_obj_set_scrollbar_mode(weightHeader, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* weightHeaderTitle = lv_label_create(weightHeader);
    lv_label_set_text(weightHeaderTitle, "Product Weight (kg)");
    lv_obj_set_style_text_font(weightHeaderTitle, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(weightHeaderTitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(weightHeaderTitle, LV_ALIGN_LEFT_MID, 0, 0);

    weightCurLabel = lv_label_create(weightHeader);
    lv_label_set_text(weightCurLabel, "-- kg");
    lv_obj_set_style_text_font(weightCurLabel, FONT_LARGE, 0);
    lv_obj_set_style_text_color(weightCurLabel, COLOR_CHART_WEIGHT, 0);
    lv_obj_align(weightCurLabel, LV_ALIGN_RIGHT_MID, 0, 0);

    weightChart = createStyledChart(weightTab, &weightSeries, COLOR_CHART_WEIGHT, 0, 10);

    return scr;
}

void updateAnalyticsScreen() {
    if (!tempChart) return;

    // Add new data points to charts
    lv_chart_set_next_value(tempChart, tempSeries, (lv_coord_t)dryerData.temperature);
    lv_chart_set_next_value(humChart, humSeries, (lv_coord_t)dryerData.humidity);

    // Weight in grams/10 for better chart resolution (0-10kg range)
    lv_coord_t weightVal = (lv_coord_t)(dryerData.weight * 10.0f);
    lv_chart_set_next_value(weightChart, weightSeries, weightVal);
    // Adjust weight chart range dynamically
    lv_coord_t maxW = (lv_coord_t)(dryerData.weight * 15.0f);
    if (maxW < 50) maxW = 50;
    lv_chart_set_range(weightChart, LV_CHART_AXIS_PRIMARY_Y, 0, maxW);

    // Refresh charts
    lv_chart_refresh(tempChart);
    lv_chart_refresh(humChart);
    lv_chart_refresh(weightChart);

    // Update current value labels
    { char _b[20]; snprintf(_b, sizeof(_b), "%.1f \xC2\xB0C", dryerData.temperature); lv_label_set_text(tempCurLabel, _b); }
    { char _b[12]; snprintf(_b, sizeof(_b), "%.0f %%RH", dryerData.humidity);          lv_label_set_text(humCurLabel, _b); }
    { char _b[12]; snprintf(_b, sizeof(_b), "%.2f kg", dryerData.weight);               lv_label_set_text(weightCurLabel, _b); }
}
