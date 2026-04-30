// boot_screen.h
// Standalone bootscreen sample for Fish Dryer V2 HMI

#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include <lvgl.h>
#include "FS.h"
#include "SD.h"

// Create and display the boot screen with SD card image support
lv_obj_t* createBootScreen(fs::FS *sdfs = nullptr);

// Load PNG image from SD card into LVGL
lv_img_dsc_t* loadImageFromSD(fs::FS *fs, const char* path);

#endif // BOOT_SCREEN_H
