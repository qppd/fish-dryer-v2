// PID_CONFIG.h
// PID controller configuration for temperature control using SHT31 and SSR relays

#ifndef PID_CONFIG_H
#define PID_CONFIG_H

#include <PID_v1.h>
#include "PINS_CONFIG.h"
#include "SSR_CONFIG.h"
#include "SHT31_CONFIG.h"

// PID tuning parameters
#define PID_KP 4.0
#define PID_KI 0.0
#define PID_KD 22.0

// PID variables
extern double CURRENT_TEMPERATURE;
extern double TEMPERATURE_SETPOINT;
double PID_OUTPUT = 0;

PID pid(&CURRENT_TEMPERATURE, &PID_OUTPUT, &TEMPERATURE_SETPOINT, PID_KP, PID_KI, PID_KD, DIRECT);

// Initialize PID controller
void initPID() {
  pid.SetMode(AUTOMATIC);
  pid.SetOutputLimits(0, 5000); // Adjust limits for your system
}

// Relay control function (to be implemented in main sketch)
extern void operateSSR(int relayIndex, bool state);

// Compute PID and control relays
void pidCOMPUTE() {
  pid.Compute();
  if (PID_OUTPUT > 0) {
    operateSSR(1, true);  // Heating Element ON
    operateSSR(2, true);  // Convection Fan ON
    operateSSR(3, false); // Exhaust Fan OFF
  } else {
    operateSSR(1, false); // Heating Element OFF
    operateSSR(2, false); // Convection Fan OFF
    operateSSR(3, true);  // Exhaust Fan ON
  }
}

#endif // PID_CONFIG_H
