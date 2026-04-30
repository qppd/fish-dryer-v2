#include "ui_optimistic_state.h"
#include <Arduino.h>

UiOptimisticState gUiOptimisticState = {
    false,
    0,
    STATE_IDLE,
    false,
    false,
    false,
    true,
    false
};

void uiOptimisticSet(DryerState state, bool heaterOn, bool fanOn, bool exhaustOn,
                     bool showStart, bool showStop, unsigned long graceMs) {
    gUiOptimisticState.active = true;
    gUiOptimisticState.untilMs = millis() + graceMs;
    gUiOptimisticState.systemState = state;
    gUiOptimisticState.heaterOn = heaterOn;
    gUiOptimisticState.fanOn = fanOn;
    gUiOptimisticState.exhaustOn = exhaustOn;
    gUiOptimisticState.showStart = showStart;
    gUiOptimisticState.showStop = showStop;
}

bool uiOptimisticIsActive() {
    if (gUiOptimisticState.active && millis() >= gUiOptimisticState.untilMs) {
        gUiOptimisticState.active = false;
    }
    return gUiOptimisticState.active;
}

void uiOptimisticClear() {
    gUiOptimisticState.active = false;
}
