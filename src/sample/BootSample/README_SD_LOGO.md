# Boot Screen Sample with SD Card Logo Support

A standalone demonstration of the Fish Dryer V2 bootscreen with full animations and SD card image support.

## Quick Start

### 1. Prepare SD Card
Place your logo image on the SD card:
```
SD Card Root:
  └── solaraw.png  (your 200x200 PNG logo)
```

### 2. Upload and Run
- Upload `BootSample.ino` to your ESP32-S3
- Monitor serial output at 115200 baud
- Bootscreen displays with custom logo image
- Falls back to symbol if image not found

## Files Included

- **BootSample.ino** - Main sketch with SD card initialization
- **boot_screen.h** - Header with SD card support
- **boot_screen.cpp** - Implementation with image loader
- **ui_theme.h** - Color and font theme
- **ui_styles.h/.cpp** - Style initialization

## Features

✨ **With SD Card Logo:**
- ⚡ Custom PNG image from `/solaraw.png`
- 🔄 Pulsing logo animation (3 pulses)
- 📝 Fading title animation
- 📊 Animated progress bar (0-100%)
- 📋 Dynamic status text
- ⚙️ **Automatic fallback** to symbol if image missing

## Hardware Requirements

- **ESP32-S3** with built-in LCD
- **LVGL v8** library
- **SD Card** (optional)

## LVGL Configuration (lv_conf.h)

Enable required fonts:
```c
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_48 1
```

Enable image support:
```c
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_IMG 1           // Image widget
#define LV_USE_TIMER 1
#define LV_USE_PNG 1           // PNG decoder
```

## Image Loading Details

### How loadImageFromSD() Works

```cpp
lv_img_dsc_t* loadImageFromSD(fs::FS *fs, const char* path);
```

1. Opens file from SD card
2. Reads entire PNG into memory buffer
3. Creates LVGL image descriptor
4. Returns descriptor for display

### Fallback Mechanism

If any step fails:
- ❌ SD card not available → Lightning bolt symbol
- ❌ `/solaraw.png` not found → Lightning bolt symbol
- ❌ Image loading fails → Lightning bolt symbol
- ✅ Success → Custom PNG image displayed

### Memory Usage

- PNG buffer: ~50-100 KB
- Image descriptor: ~100 bytes
- Total heap impact: minimal

## Serial Output Example

```
=== Boot Screen Sample with SD Card Logo ===
[BOOT] Initializing SD card...
[BOOT] SD Card Type: SDHC
[BOOT] SD Card Size: 8192MB
[BOOT] Initializing display board...
[BOOT] Initializing LVGL...
[BOOT] Building UI...
[BOOT] Loading image from: /solaraw.png
[BOOT] Image file size: 45000 bytes
[BOOT] Image loaded successfully (200x200)
[BOOT] Using PNG logo from SD card
[BOOT] Boot screen displayed
[BOOT] Setup complete!
[BOOT] Free heap: 1024 KB
[BOOT] Boot sequence complete!
```

## Customization

### Change Logo Filename

In `boot_screen.cpp`:
```cpp
lv_img_dsc_t* imgDsc = loadImageFromSD(sdfs, "/your_logo.png");
```

### Adjust Image Dimensions

In `boot_screen.cpp`, edit image descriptor:
```cpp
logoImage->header.w = 250;  // Width
logoImage->header.h = 250;  // Height
```

### Extend Boot Duration

In `boot_screen.cpp`:
```cpp
lv_timer_t* boot_timer = lv_timer_create(bootCompleteCallback, 5000, NULL);  // 5 seconds
```

### Skip SD Card (Use Symbol Only)

In `BootSample.ino` setup:
```cpp
lv_obj_t* bootScr = createBootScreen(NULL);  // Pass NULL instead of &SD
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| SD card not detected | Check pin definitions, format as FAT32 |
| Image not loading | Verify `/solaraw.png` exists in root, check serial output |
| Screen blank | Enable LVGL fonts in `lv_conf.h` |
| Memory warning | Reduce image size or resolution |
| No animations | Ensure LVGL timer support enabled |

## Integration with Main App

```cpp
// Initialize SD card
SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_SS);
SD.begin(SD_SS);

// Initialize display and LVGL
lvgl_port_init(board->getLCD(), board->getTouch());

// Create bootscreen with SD support
lvgl_port_lock(-1);
initStyles();
lv_obj_t* bootScr = createBootScreen(&SD);  // Pass SD filesystem
lv_scr_load(bootScr);
lvgl_port_unlock();

// Later, in bootCompleteCallback or timer:
loadScreen(SCREEN_DASHBOARD);  // Transition to main app
```

## API Reference

### createBootScreen()
```cpp
lv_obj_t* createBootScreen(fs::FS *sdfs = nullptr);
```
- **sdfs**: SD filesystem pointer (or `nullptr` for fallback)
- **Returns**: LVGL screen object

### loadImageFromSD()
```cpp
lv_img_dsc_t* loadImageFromSD(fs::FS *fs, const char* path);
```
- **fs**: Filesystem object
- **path**: File path (e.g., "/solaraw.png")
- **Returns**: LVGL image descriptor or `nullptr` on error

## License

Part of the Fish Dryer V2 HMI system (April 2026)
