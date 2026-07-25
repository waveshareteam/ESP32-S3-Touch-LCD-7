#pragma once

#include <Arduino.h>
#include <esp_err.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

typedef lv_disp_t lv_display_t;

typedef enum {
    ESP_LV_ADAPTER_ROTATE_0 = 0,
} esp_lv_adapter_rotation_t;

typedef enum {
    ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB = 0,
} esp_lv_adapter_tear_avoid_mode_t;

typedef struct {
    uint32_t task_stack_size;
    uint32_t task_priority;
    int task_core_id;
    uint32_t tick_period_ms;
    uint32_t task_min_delay_ms;
    uint32_t task_max_delay_ms;
    bool stack_in_psram;
} esp_lv_adapter_config_t;

#ifdef CONFIG_ARDUINO_RUNNING_CORE
#define ESP_LV_ADAPTER_DEFAULT_TASK_CORE_ID CONFIG_ARDUINO_RUNNING_CORE
#else
#define ESP_LV_ADAPTER_DEFAULT_TASK_CORE_ID ARDUINO_RUNNING_CORE
#endif

#define ESP_LV_ADAPTER_DEFAULT_CONFIG() \
    { 8 * 1024, 2, ESP_LV_ADAPTER_DEFAULT_TASK_CORE_ID, 2, 2, 500, false }

typedef struct {
    bool use_psram;
} esp_lv_adapter_profile_t;

typedef struct {
    esp_panel::drivers::LCD *lcd;
    void *panel_io;
    uint16_t hor_res;
    uint16_t ver_res;
    esp_lv_adapter_rotation_t rotation;
    esp_lv_adapter_profile_t profile;
} esp_lv_adapter_display_config_t;

#define ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(lcd, hor_res, ver_res, rotation) \
    { (lcd), nullptr, (hor_res), (ver_res), (rotation), { false } }

typedef struct {
    lv_display_t *disp;
    esp_panel::drivers::Touch *touch;
} esp_lv_adapter_touch_config_t;

#define ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch) \
    { (disp), (touch) }

#ifdef __cplusplus
extern "C" {
#endif

uint8_t esp_lv_adapter_get_required_frame_buffer_count(
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode, esp_lv_adapter_rotation_t rotation
);
esp_err_t esp_lv_adapter_init(const esp_lv_adapter_config_t *config);
esp_err_t esp_lv_adapter_start(void);
esp_err_t esp_lv_adapter_deinit(void);
esp_err_t esp_lv_adapter_lock(int32_t timeout_ms);
void esp_lv_adapter_unlock(void);
lv_display_t *esp_lv_adapter_register_display(const esp_lv_adapter_display_config_t *config);
lv_indev_t *esp_lv_adapter_register_touch(const esp_lv_adapter_touch_config_t *config);

#ifdef __cplusplus
}
#endif
