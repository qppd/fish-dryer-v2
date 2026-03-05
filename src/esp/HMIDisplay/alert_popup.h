// alert_popup.h
// Fish Dryer V2 HMI - Alert and notification popup system

#ifndef ALERT_POPUP_H
#define ALERT_POPUP_H

#include <lvgl.h>

// Alert types
enum AlertType {
    ALERT_INFO,
    ALERT_WARNING,
    ALERT_ERROR,
    ALERT_SUCCESS
};

// Initialize the alert system
void alertInit();

// Show an alert popup overlay
void showAlert(const char* title, const char* message, AlertType type);

// Show the "Drying Complete" screen overlay
void showDryingComplete(float waterLoss, unsigned long elapsedMs);

// Dismiss any active alert
void dismissAlert();

// Check and trigger alerts based on dryer data (call periodically)
void checkAlerts();

#endif // ALERT_POPUP_H
