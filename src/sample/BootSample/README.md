# Boot Screen Sample

A standalone demonstration of the Fish Dryer V2 bootscreen with full animations.

## Files Included

- **BootSample.ino** - Main Arduino sketch (entry point)
- **boot_screen.h** - Bootscreen header
- **boot_screen.cpp** - Bootscreen implementation
- **ui_theme.h** - Color and font theme definitions
- **ui_styles.h** - Style definitions header
- **ui_styles.cpp** - Style initialization

## What It Does

This sample displays a professional bootscreen with:
- ⚡ Pulsing heat icon (3 pulses)
- 📝 Fading title animation ("SolAraw")
- 📊 Animated progress bar (0-100%)
- 📋 Status text updates matching progress
- 🔢 Version display
- ⏱️ 3-second boot sequence

The screen automatically transitions at the end via timer callback.

## Hardware Requirements

- **ESP32-S3** with built-in LCD (e.g., Waveshare ESP32-S3-Touch-LCD-7)
- **Resolution**: 800x480 or compatible
- **LVGL v8** library configured

## LVGL Configuration (lv_conf.h)

Enable these fonts for proper display:
```c
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_48 1
```

Ensure LVGL display and timer support:
```c
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_TIMER 1
```

## Compilation

This sketch is designed to work standalone with the Arduino IDE:

1. Copy the entire `BootSample` folder to your Arduino projects
2. Install required libraries:
   - LVGL (via Library Manager)
   - esp-display-panel (if using ESP32-S3-LCD board)
3. Select your ESP32 board and USB port
4. Upload the sketch

## Serial Output

When running, you'll see boot progress on Serial (115200 baud):
```
=== Boot Screen Sample ===
[BOOT] Initializing display board...
[BOOT] Initializing LVGL...
[BOOT] Building UI...
[BOOT] Boot screen displayed
[BOOT] Setup complete!
[BOOT] Free heap: 1024 KB
[BOOT] Boot screen will display for 3 seconds with animations...
[BOOT] Boot sequence complete!
```

## Customization

### Change Title Text
Edit in `boot_screen.cpp`:
```cpp
lv_label_set_text(title, "Your App Name");
```

### Change Boot Duration
Edit the timer delay in `boot_screen.cpp`:
```cpp
lv_timer_t* boot_timer = lv_timer_create(bootCompleteCallback, 5000, NULL);  // 5 seconds
```

### Change Progress Bar Duration
Edit animation time:
```cpp
lv_anim_set_time(&boot_bar_anim, 4000);  // Match timer duration
```

### Change Colors
Edit `ui_theme.h`:
```cpp
#define COLOR_ACCENT lv_color_hex(0xFF6B00)  // Your color here
```

## Troubleshooting

**Screen appears blank:**
- Verify LVGL fonts are enabled in `lv_conf.h`
- Check serial output for error messages
- Confirm display board is initialized correctly

**Animations don't show:**
- Ensure LVGL timer support is enabled
- Verify FPS is sufficient (~60Hz recommended)

**Memory issues:**
- This sample uses ~500KB of heap
- Check `Free heap` output in serial monitor

## Integration with Main App

To use this bootscreen in your main Fish Dryer app:

1. Copy `boot_screen.h/cpp`, `ui_styles.h/cpp`, `ui_theme.h` to your project
2. Call `initStyles()` in your setup
3. Call `createBootScreen()` to create the bootscreen
4. Use `lv_scr_load()` to display it
5. In the boot complete callback, transition to your main screen:
   ```cpp
   loadScreen(SCREEN_DASHBOARD);  // or your main screen
   ```

## License

Part of the Fish Dryer V2 HMI system (March 2026)
