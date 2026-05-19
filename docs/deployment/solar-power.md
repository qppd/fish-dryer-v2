# Solar Power System & Deployment

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Power System Overview

The Fish Dryer v2 is designed for **off-grid solar operation** with mains backup.

```
2× 300W Solar Panels (600W total)
    │
    ▼
40A MPPT Charge Controller
    │
    ▼
300Ah LiFePO4 Battery (12V, 3600Wh)
    │
    ├── DC Breakers (40A main)
    │       │
    │       ├── 3000W Pure Sine Inverter → AC loads
    │       │                                  │
    │       │                                  ├── Fish Dryer (650-1300W)
    │       │                                  └── Other AC loads
    │       │
    │       └── LVD (Low Voltage Disconnect) → Battery protection
    │
    └── 5V/3.3V regulators → ESP32/Nano/HMI
```

## 2. Component Specifications

### 2.1 Solar Panels

| Parameter | Value |
|-----------|-------|
| Quantity | 2 |
| Panel Power | 300W each (600W total) |
| Type | Monocrystalline |
| System Voltage | 12V nominal |
| Cabling | 10 AWG minimum |

### 2.2 MPPT Charge Controller

| Parameter | Value |
|-----------|-------|
| Type | MPPT (Maximum Power Point Tracking) |
| Current Rating | 40A |
| Efficiency | 95%+ |

### 2.3 Battery Bank

| Parameter | Value |
|-----------|-------|
| Type | LiFePO4 (Lithium Iron Phosphate) |
| Capacity | 300Ah at 12V = 3600Wh |
| Cycle Life | 2000+ cycles |
| BMS | Built-in Battery Management System |
| Charge Voltage | 14.2-14.6V (bulk), 13.6V (float) |
| Max Charge Current | 40A (MPPT limited) |

### 2.4 Inverter

| Parameter | Value |
|-----------|-------|
| Type | Pure Sine Wave |
| Continuous Power | 3000W |
| Input | 12V DC |

## 3. Power Consumption

| Component | Max Power | Duty Cycle | Average |
|-----------|-----------|------------|---------|
| ESP32 Controller | 500mW | 100% | 500mW |
| SHT31 Sensor | 1.5mW | 100% | 1.5mW |
| HX711 + Load Cell | 10mW | 100% | 10mW |
| Heating Element | 1000-2000W | 40-60% | 600-1200W |
| Convection Fan | 20-50W | 60-80% | 16-40W |
| Exhaust Fan | 20-50W | 20-40% | 4-20W |
| **Total System** | **1100-2100W** | Variable | **650-1300W** |

## 4. Runtime Estimates

| Scenario | Draw | Runtime (300Ah) |
|----------|------|-----------------|
| Low (fans only) | 650W | ~5.5 hours |
| Medium (balanced) | 1000W | ~3.6 hours |
| High (max heat) | 1300W | ~2.8 hours |
| Solar recharge (600W) | — | 2-4 hours full charge |

## 5. Safety Systems

### 5.1 Electrical Safety

| Feature | Purpose |
|---------|---------|
| DC Breakers (40A main) | Overcurrent protection |
| LVD | Prevents deep battery discharge (~10.5V cutoff) |
| SSR Optocouplers | Galvanic isolation (logic ↔ high voltage) |

### 5.2 Software Safety

| Feature | Trigger | Action |
|---------|---------|--------|
| Temperature sensor fail | 3× consecutive NaN | Heater OFF, exhaust ON |
| Overheat | Temperature > 85°C (configurable) | PID disabled |
| Critical temperature | > 110°C | Emergency shutdown |
| Connection lost | 6s without status | HMI alert |

## 6. Deployment Checklist

### Before First Run

- [ ] All three microcontrollers flashed and verified
- [ ] ESP-Now MAC pairing completed
- [ ] SHT31 sensor detected at boot
- [ ] HX711 load cell responds and tares
- [ ] All SSR relays click when commanded
- [ ] Voltage divider in place (Nano D10 → NodeMCU D5)
- [ ] Common ground established between all nodes
- [ ] Chamber empty, door closed
- [ ] Send `STATUS?` — full JSON response received

### Production Environment

- [ ] Battery at full charge (13.6V+)
- [ ] Solar panels connected and charging
- [ ] DC breakers ON
- [ ] Inverter ON (if using AC heater)
- [ ] ESP32/Nano powered
- [ ] HMI display showing Dashboard
- [ ] Check WiFi icon = green (connected)

### Monthly Maintenance

- [ ] Re-calibrate load cell with known weight
- [ ] Clean exhaust filter
- [ ] Check SHT31 accuracy compared to reference thermometer
- [ ] Inspect SSR heatsinks for dust
- [ ] Test auto-stop with simulation
- [ ] Verify ESP-Now link quality
