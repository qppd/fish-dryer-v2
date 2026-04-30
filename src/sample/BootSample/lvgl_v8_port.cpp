/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 * 
 * Simplified version for BootSample standalone compilation
 */

#include "esp_timer.h"
#include "esp_lib_utils.h"
#include "lvgl_v8_port.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

using namespace esp_panel::drivers;

static SemaphoreHandle_t lvgl_mux = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static esp_timer_handle_t lvgl_tick_timer = NULL;
static void *lvgl_buf[LVGL_PORT_BUFFER_NUM] = {};

#if !LV_TICK_CUSTOM
static void tick_increment(void *arg) {
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

static bool tick_init(void) {
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &tick_increment,
        .name = "LVGL tick"
    };
    if (esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer) != ESP_OK) return false;
    if (esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000) != ESP_OK) return false;
    return true;
}

static bool tick_deinit(void) {
    if (esp_timer_stop(lvgl_tick_timer) != ESP_OK) return false;
    if (esp_timer_delete(lvgl_tick_timer) != ESP_OK) return false;
    return true;
}
#endif

static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    LCD *lcd = (LCD *)drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    lcd->drawBitmap(offsetx1, offsety1, offsetx2 - offsetx1 + 1, offsety2 - offsety1 + 1, (const uint8_t *)color_map);
    lv_disp_flush_ready(drv);
}

void rounder_callback(lv_disp_drv_t *drv, lv_area_t *area) {
    LCD *lcd = (LCD *)drv->user_data;
    uint8_t x_align = lcd->getBasicAttributes().basic_bus_spec.x_coord_align;
    uint8_t y_align = lcd->getBasicAttributes().basic_bus_spec.y_coord_align;

    if (x_align > 1) {
        area->x1 &= ~(x_align - 1);
        area->x2 = (area->x2 & ~(x_align - 1)) + x_align - 1;
    }
    if (y_align > 1) {
        area->y1 &= ~(y_align - 1);
        area->y2 = (area->y2 & ~(y_align - 1)) + y_align - 1;
    }
}

static lv_disp_t *display_init(LCD *lcd) {
    if (lcd == nullptr || lcd->getRefreshPanelHandle() == nullptr) return nullptr;

    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    auto lcd_width = lcd->getFrameWidth();
    auto lcd_height = lcd->getFrameHeight();
    int buffer_size = lcd_width * LVGL_PORT_BUFFER_SIZE_HEIGHT;

#if LVGL_PORT_AVOID_TEARING_MODE == 0
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM; i++) {
        lvgl_buf[i] = heap_caps_malloc(buffer_size * sizeof(lv_color_t), LVGL_PORT_BUFFER_MALLOC_CAPS);
        if (!lvgl_buf[i]) return nullptr;
    }
#else
    buffer_size = lcd_width * lcd_height;
#if LVGL_PORT_DISP_BUFFER_NUM >= 2
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM; i++) {
        lvgl_buf[i] = lcd->getFrameBufferByIndex(i);
    }
#endif
#endif

    lv_disp_draw_buf_init(&disp_buf, lvgl_buf[0], lvgl_buf[1], buffer_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = flush_callback;
    disp_drv.hor_res = lcd_width;
    disp_drv.ver_res = lcd_height;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = (void *)lcd;
    
    if ((lcd->getBasicAttributes().basic_bus_spec.x_coord_align > 1) ||
            (lcd->getBasicAttributes().basic_bus_spec.y_coord_align > 1)) {
        disp_drv.rounder_cb = rounder_callback;
    }

    return lv_disp_drv_register(&disp_drv);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    Touch *tp = (Touch *)indev_drv->user_data;
    TouchPoint point;

    int read_touch_result = tp->readPoints(&point, 1, 0);
    if (read_touch_result > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_indev_t *indev_init(Touch *tp) {
    if (tp == nullptr || tp->getPanelHandle() == nullptr) return nullptr;

    static lv_indev_drv_t indev_drv_tp;

    lv_indev_drv_init(&indev_drv_tp);
    indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
    indev_drv_tp.read_cb = touchpad_read;
    indev_drv_tp.user_data = (void *)tp;

    return lv_indev_drv_register(&indev_drv_tp);
}

static void lvgl_port_task(void *arg) {
    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        if (task_delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

bool lvgl_port_init(LCD *lcd, Touch *tp) {
    if (lcd == nullptr) return false;

    lv_disp_t *disp = nullptr;
    lv_indev_t *indev = nullptr;

    lv_init();
#if !LV_TICK_CUSTOM
    if (!tick_init()) return false;
#endif

    disp = display_init(lcd);
    if (disp == nullptr) return false;
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    if (tp != nullptr) {
        indev = indev_init(tp);
        if (indev == nullptr) return false;
    }

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (lvgl_mux == nullptr) return false;

    BaseType_t core_id = (LVGL_PORT_TASK_CORE < 0) ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;
    BaseType_t ret = xTaskCreatePinnedToCore(lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, NULL,
                     LVGL_PORT_TASK_PRIORITY, &lvgl_task_handle, core_id);
    if (ret != pdPASS) return false;

    return true;
}

bool lvgl_port_lock(int timeout_ms) {
    if (lvgl_mux == nullptr) return false;
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE);
}

bool lvgl_port_unlock(void) {
    if (lvgl_mux == nullptr) return false;
    xSemaphoreGiveRecursive(lvgl_mux);
    return true;
}

bool lvgl_port_deinit(void) {
#if !LV_TICK_CUSTOM
    if (!tick_deinit()) return false;
#endif

    if (!lvgl_port_lock(-1)) return false;
    if (lvgl_task_handle != nullptr) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = nullptr;
    }
    if (!lvgl_port_unlock()) return false;

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif

#if !LVGL_PORT_AVOID_TEAR
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM; i++) {
        if (lvgl_buf[i] != nullptr) {
            free(lvgl_buf[i]);
            lvgl_buf[i] = nullptr;
        }
    }
#endif

    if (lvgl_mux != nullptr) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = nullptr;
    }

    return true;
}
