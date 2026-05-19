# Drying Process Flow

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Complete Drying Cycle

```
[IDLE] ──[START pressed/PID_START cmd]──► [DRYING]
                                              │
                    ┌─────────────────────────┤
                    │                         │
                    ▼                         ▼
            [Auto-Stop:                [Manual Stop:
             Water Loss ≥ Target]       PID_STOP cmd]
                    │                         │
                    ▼                         │
            [COMPLETE] ◄──────────────────────┘
                    │
                    │
                    ▼
              Return to [IDLE]
              (manual reset / STOP cmd)
```

## 2. Starting a Drying Cycle

### User actions:
1. **Prepare product** — Clean, gut, arrange on tray
2. **Load tray** — Place on load cell platform inside chamber
3. **Close door** — Ensure chamber is sealed
4. **Tare (optional)** — Diagnostics screen → TARE SCALE
5. **Set temperature** — Select preset or manual temperature
6. **Set water loss target** — Slider 10-95%
7. **Press START** — On Dashboard or Control screen

### System sequence on START:

```
1. Optimistic UI: Immediate visual feedback
   ├── Button changes: START hidden → STOP shown
   ├── State badge: "DRYING" displayed
   ├── Relays: Heater=ON, Fan=ON, Exhaust=OFF
   └── Timer: 15-second grace period

2. ESP-Now: sendCmd(CMD_START_DRYING) → NodeMCU

3. NodeMCU: nano.println("PID_START")

4. Nano:
   ├── INITIAL_WEIGHT = CURRENT_WEIGHT (baseline)
   ├── systemState = STATE_DRYING
   ├── PID_ENABLED = true
   └── dryingStartMs = millis()

5. Next PID cycle (within 2000ms):
   ├── PID.Compute() → PID_OUTPUT
   ├── SSR1=ON, SSR2=ON, SSR3=OFF (heating)
   └── Chamber begins to warm up
```

## 3. Active Drying Phase

### 2000ms cycle (repeating):

```
┌─────────────────────────────────────────────────────┐
│                  2000ms CYCLE                        │
├─────────────────────────────────────────────────────┤
│                                                      │
│  1. updateSHT31()                                    │
│     └─ CURRENT_TEMPERATURE = sensor temp             │
│     └─ CURRENT_HUMIDITY = sensor humidity            │
│                                                      │
│  2. readLoadCell()                                   │
│     └─ CURRENT_WEIGHT = averaged weight (kg)         │
│                                                      │
│  3. updateWaterLoss()                                │
│     └─ WATER_LOSS = (initial - current)/initial × 100│
│                                                      │
│  4. pidCOMPUTE()                                     │
│     └─ PID.Compute(CurrentTemp)                      │
│     └─ if output > 0: SSR1=ON, SSR2=ON, SSR3=OFF     │
│     └─ if output ≤ 0: SSR1=OFF, SSR2=OFF, SSR3=ON    │
│                                                      │
│  5. Check auto-stop                                   │
│     └─ Nano: if WATER_LOSS >= TARGET → COMPLETE      │
│     └─ ESP32: also humidity <= 14% for 5 checks       │
│                                                      │
│  6. EDT update (ESP32, every 10 min)                 │
│     └─ Calculate drying rate, project completion     │
│                                                      │
│  7. Status sent to HMI via NodeMCU bridge             │
│     └─ NodeMCU polls STATUS? every 2000ms            │
│     └─ JSON parsed → EspNowStatusPacket → ESP-Now    │
│     └─ HMI updates gauge, charts, indicators          │
└─────────────────────────────────────────────────────┘
```

## 4. Auto-Stop Triggers

### Trigger A: Water Loss Target Reached

```
Condition: CURRENT_WATER_LOSS >= WATER_LOSS_TARGET
Action:   stopDrying()
          ├── systemState = COMPLETE
          ├── PID_ENABLED = false
          └── All SSR relays OFF

HMI response:
  ├── State badge → "COMPLETE" (green)
  ├── Alert overlay → "DRYING COMPLETE"
  │     └─ Shows: Water Loss %, Elapsed Time
  └── START/STOP buttons hidden
```

### Trigger B: Humidity Threshold (ESP32 only)

```
Condition: CURRENT_HUMIDITY <= 14% for 5 consecutive readings
Action:   stopDrying()
          └─ Same as Trigger A
```

## 5. Manual Stop

```
User presses STOP → immediate:
  ├── Optimistic UI: START shown, STOP hidden
  ├── systemState = IDLE
  ├── PID_ENABLED = false
  ├── All SSR relays OFF
  └── sendCmd(CMD_STOP_DRYING)
```

## 6. Pause / Resume

```
PAUSE:
  ├── PID_ENABLED = false
  ├── systemState = PAUSED
  └── Relays remain in current state

RESUME:
  ├── (only from PAUSED state)
  ├── PID_ENABLED = true
  └── systemState = DRYING
```

## 7. Drying Complete (User Actions)

1. HMI displays "DRYING COMPLETE" overlay
2. User taps ACKNOWLEDGE
3. Wait 5 minutes for chamber to cool
4. Remove dried product
5. Store in airtight containers
6. Clean chamber for next batch

## 8. Normal vs. Emergency Stop

| Type | Trigger | SSR1 | SSR2 | SSR3 | State |
|------|---------|------|------|------|-------|
| Auto-Stop (Normal) | Water loss ≥ target | OFF | OFF | OFF | COMPLETE |
| Auto-Stop (Humidity) | Humidity ≤14% | OFF | OFF | OFF | COMPLETE |
| Manual Stop | HMI STOP button | OFF | OFF | OFF | IDLE |
| Sensor Failure | 3× consecutive SHT31 fail | OFF | OFF | ON | ERROR |
| Emergency | Temperature > 110°C | OFF | OFF | ON | ERROR |
