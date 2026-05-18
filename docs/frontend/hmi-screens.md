# HMI Screens — LVGL Touch Interface

**Author:** Sajed Lopez Mendoza — QPPD

---

## 1. Screen Map

```
┌─────────────┐
│  SCREEN_BOOT│  (3-second splash)
└──────┬──────┘
       │ Auto-transition (3s timer)
       ▼
┌─────────────┐
│  DASHBOARD  │  ←─── HOME SCREEN
└──┬──┬──┬──┬─┘
   │  │  │  │
   │  │  │  └──→ Diagnostics (SCREEN_DIAGNOSTICS)
   │  │  │
   │  │  └─────→ Analytics (SCREEN_ANALYTICS)
   │  │
   │  └────────→ Control Panel (SCREEN_CONTROL)
   │
   └───────────→ [START/STOP buttons]
```

All sub-screens have a **Back button** (←) in the top bar that returns to Dashboard.

## 2. Screen Configuration

| Parameter | Value |
|-----------|-------|
| Display resolution | 800 × 480 px |
| LVGL version | 8.3.x |
| Color depth | RGB565 (16-bit) |
| Screen transitions | Fade ON, 300ms |
| Auto-delete previous screen | Yes (auto_del = true) |

## 3. Layout Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `TOP_BAR_HEIGHT` | 50 px | Header bar |
| `BOTTOM_BAR_HEIGHT` | 60 px | Footer bar (dashboard only) |
| `CONTENT_HEIGHT` | 370 px | Main content area |
| `SIDE_PADDING` | 15 px | Horizontal margins |
| `WIDGET_SPACING` | 10 px | Between widgets |
| `BTN_MIN_SIZE` | 48 px | Minimum button touch target |

## 4. LCD Panel Configuration

| Parameter | Value |
|-----------|-------|
| Board | Waveshare ESP32-S3-Touch-LCD-7 |
| LCD Controller | ST7262 |
| Bus Type | RGB (ESP_PANEL_BUS_TYPE_RGB) |
| Data Width | 16-bit (RGB565) |
| PCLK | 16 MHz |
| HSYNC Pins | GPIO 46 |
| VSYNC Pins | GPIO 3 |
| DE Pin | GPIO 5 |
| PCLK Pin | GPIO 7 |
| Data Pins | GPIO 14,38,18,17,10,39,0,45,48,47,21,1,2,42,41,40 |

## 5. Touch Panel Configuration

| Parameter | Value |
|-----------|-------|
| Controller | GT911 |
| Bus Type | I2C (400 kHz) |
| I2C Pins | SCL=GPIO9, SDA=GPIO8 |
| Interrupt | GPIO 4 |
| Touch Reset | CH422G IO-1 |

## 6. IO Expander (CH422G)

| IO | Function |
|----|----------|
| IO-0 | LCD Backlight control (switch) |
| IO-1 | Touch RST |
| IO-3 | LCD RST |

## 7. LVGL Port Configuration

| Parameter | Value |
|-----------|-------|
| Tick period | 2 ms |
| Buffer allocation | `MALLOC_CAP_INTERNAL` + `MALLOC_CAP_8BIT` |
| Buffer height | 20 lines |
| Buffer count | 2 |
| Task stack size | 10 KB |
| Task priority | 2 |
| Task core | Arduino running core |
| Avoid tearing mode | 3 (double-buffer + direct mode) |
| Max task delay | 500 ms |
| Min task delay | 2 ms |

## 8. Fonts Used

| Font Name | Size | Usage |
|-----------|------|-------|
| `lv_font_montserrat_16` | 16px | Labels, status text, secondary |
| `lv_font_montserrat_20` | 20px | Buttons, medium text |
| `lv_font_montserrat_24` | 24px | Card titles, large labels |
| `lv_font_montserrat_30` | 30px | Medium values, metrics |
| `lv_font_montserrat_36` | 36px | Large metrics (humidity, weight) |
| `lv_font_montserrat_48` | 48px | Dashboard temperature, huge icons |
