#pragma once

#include <Arduino.h>

#include <esp_display_panel.hpp>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LVGL_V9_PORT_TICK_PERIOD_MS      (2)
#define LVGL_V9_PORT_TASK_MIN_DELAY_MS   (2)
#define LVGL_V9_PORT_TASK_MAX_DELAY_MS   (500)
#define LVGL_V9_PORT_TASK_STACK_SIZE     (8 * 1024)
#define LVGL_V9_PORT_TASK_PRIORITY       (2)
#define LVGL_V9_PORT_FRAME_BUFFER_NUM    (2)
#define LVGL_V9_PORT_BUFFER_LINES        (20)

bool lvgl_v9_port_init(esp_panel::drivers::LCD *lcd, esp_panel::drivers::Touch *tp);
bool lvgl_v9_port_lock(int timeout_ms);
bool lvgl_v9_port_unlock(void);

#ifdef __cplusplus
}
#endif
