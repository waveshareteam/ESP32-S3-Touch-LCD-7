#include "esp_lv_adapter_arduino.h"

#include <string.h>
#include <limits.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

using namespace esp_panel::drivers;

typedef struct {
    bool initialized;
    bool started;
    bool rgb_full_refresh;
    esp_lv_adapter_config_t config;
    SemaphoreHandle_t lvgl_mux;
    TaskHandle_t lvgl_task_handle;
    esp_timer_handle_t lvgl_tick_timer;
    LCD *lcd;
    Touch *touch;
    lv_display_t *display;
    lv_indev_t *indev;
    void *buf1;
    void *buf2;
    bool buf_allocated;
} esp_lv_adapter_arduino_context_t;

static esp_lv_adapter_arduino_context_t s_ctx = {};

static void lvgl_tick_cb(void *arg)
{
    LV_UNUSED(arg);
    lv_tick_inc(s_ctx.config.tick_period_ms);
}

static void lvgl_worker(void *arg)
{
    LV_UNUSED(arg);

    uint32_t delay_ms = s_ctx.config.task_max_delay_ms;
    while (true) {
        if (esp_lv_adapter_lock(-1) == ESP_OK) {
            delay_ms = lv_timer_handler();
            esp_lv_adapter_unlock();
        }

        if (delay_ms < s_ctx.config.task_min_delay_ms) {
            delay_ms = s_ctx.config.task_min_delay_ms;
        } else if (delay_ms > s_ctx.config.task_max_delay_ms) {
            delay_ms = s_ctx.config.task_max_delay_ms;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

IRAM_ATTR static bool rgb_refresh_finish_cb(void *user_data)
{
    BaseType_t need_yield = pdFALSE;
    TaskHandle_t task_handle = static_cast<TaskHandle_t>(user_data);
    xTaskNotifyFromISR(task_handle, ULONG_MAX, eNoAction, &need_yield);
    return (need_yield == pdTRUE);
}

IRAM_ATTR static bool draw_bitmap_finish_cb(void *user_data)
{
    lv_display_t *disp = static_cast<lv_display_t *>(user_data);
    lv_display_flush_ready(disp);
    return false;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    LCD *lcd = static_cast<LCD *>(lv_display_get_user_data(disp));
    if (lcd == nullptr) {
        lv_display_flush_ready(disp);
        return;
    }

    if (s_ctx.rgb_full_refresh) {
        if (lv_display_flush_is_last(disp)) {
            lcd->switchFrameBufferTo(px_map);
            ulTaskNotifyValueClear(nullptr, ULONG_MAX);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
        lv_display_flush_ready(disp);
        return;
    }

    lcd->drawBitmap(
        area->x1,
        area->y1,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1,
        px_map
    );

    if (lcd->getBus()->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        lv_display_flush_ready(disp);
    }
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    Touch *touch = static_cast<Touch *>(lv_indev_get_user_data(indev));
    TouchPoint point = {};

    if ((touch != nullptr) && (touch->readPoints(&point, 1, 0) > 0)) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
        return;
    }

    data->state = LV_INDEV_STATE_RELEASED;
}

uint8_t esp_lv_adapter_get_required_frame_buffer_count(
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode, esp_lv_adapter_rotation_t rotation
)
{
    LV_UNUSED(tear_avoid_mode);
    LV_UNUSED(rotation);
    return 2;
}

esp_err_t esp_lv_adapter_init(const esp_lv_adapter_config_t *config)
{
    if ((config == nullptr) || s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.config = *config;
    s_ctx.lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (s_ctx.lvgl_mux == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    lv_init();

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &s_ctx.lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_ctx.lvgl_tick_timer, s_ctx.config.tick_period_ms * 1000));

    s_ctx.initialized = true;
    return ESP_OK;
}

lv_display_t *esp_lv_adapter_register_display(const esp_lv_adapter_display_config_t *config)
{
    if ((!s_ctx.initialized) || (config == nullptr) || (config->lcd == nullptr) || (s_ctx.display != nullptr)) {
        return nullptr;
    }

    s_ctx.lcd = config->lcd;
    s_ctx.rgb_full_refresh = false;
    s_ctx.buf_allocated = false;
    s_ctx.buf1 = nullptr;
    s_ctx.buf2 = nullptr;

    const uint32_t width = config->hor_res;
    const uint32_t height = config->ver_res;
    const auto bus_type = s_ctx.lcd->getBus()->getBasicAttributes().type;
    uint32_t buffer_bytes = width * 40 * sizeof(lv_color_t);
    lv_display_render_mode_t render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;

    if (bus_type == ESP_PANEL_BUS_TYPE_RGB) {
        s_ctx.buf1 = s_ctx.lcd->getFrameBufferByIndex(0);
        s_ctx.buf2 = s_ctx.lcd->getFrameBufferByIndex(1);
        if ((s_ctx.buf1 != nullptr) && (s_ctx.buf2 != nullptr)) {
            buffer_bytes = width * height * sizeof(lv_color_t);
            render_mode = LV_DISPLAY_RENDER_MODE_FULL;
            s_ctx.rgb_full_refresh = true;
        }
    }

    if ((s_ctx.buf1 == nullptr) || (s_ctx.buf2 == nullptr)) {
        s_ctx.buf1 = heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        s_ctx.buf2 = heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if ((s_ctx.buf1 == nullptr) || (s_ctx.buf2 == nullptr)) {
            if (s_ctx.buf1 != nullptr) {
                free(s_ctx.buf1);
            }
            if (s_ctx.buf2 != nullptr) {
                free(s_ctx.buf2);
            }
            s_ctx.buf1 = nullptr;
            s_ctx.buf2 = nullptr;
            return nullptr;
        }
        s_ctx.buf_allocated = true;
        s_ctx.rgb_full_refresh = false;
    }

    s_ctx.display = lv_display_create(width, height);
    if (s_ctx.display == nullptr) {
        return nullptr;
    }

    lv_display_set_color_format(s_ctx.display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_ctx.display, flush_cb);
    lv_display_set_buffers(s_ctx.display, s_ctx.buf1, s_ctx.buf2, buffer_bytes, render_mode);
    lv_display_set_user_data(s_ctx.display, s_ctx.lcd);
    lv_display_set_default(s_ctx.display);

    return s_ctx.display;
}

lv_indev_t *esp_lv_adapter_register_touch(const esp_lv_adapter_touch_config_t *config)
{
    if ((config == nullptr) || (config->disp == nullptr) || (config->touch == nullptr)) {
        return nullptr;
    }

    s_ctx.touch = config->touch;
    s_ctx.indev = lv_indev_create();
    if (s_ctx.indev == nullptr) {
        return nullptr;
    }

    lv_indev_set_type(s_ctx.indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_ctx.indev, touch_read_cb);
    lv_indev_set_user_data(s_ctx.indev, s_ctx.touch);
    lv_indev_set_display(s_ctx.indev, config->disp);

    return s_ctx.indev;
}

esp_err_t esp_lv_adapter_start(void)
{
    if ((!s_ctx.initialized) || (s_ctx.display == nullptr) || s_ctx.started) {
        return ESP_ERR_INVALID_STATE;
    }

    BaseType_t core_id = (s_ctx.config.task_core_id < 0) ? tskNO_AFFINITY : s_ctx.config.task_core_id;
    BaseType_t ret = xTaskCreatePinnedToCore(
        lvgl_worker,
        "lvgl",
        s_ctx.config.task_stack_size,
        nullptr,
        s_ctx.config.task_priority,
        &s_ctx.lvgl_task_handle,
        core_id
    );
    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    if (s_ctx.rgb_full_refresh) {
        s_ctx.lcd->attachRefreshFinishCallback(rgb_refresh_finish_cb, s_ctx.lvgl_task_handle);
    } else if (s_ctx.lcd->getBus()->getBasicAttributes().type != ESP_PANEL_BUS_TYPE_RGB) {
        s_ctx.lcd->attachDrawBitmapFinishCallback(draw_bitmap_finish_cb, s_ctx.display);
    }

    s_ctx.started = true;
    return ESP_OK;
}

esp_err_t esp_lv_adapter_lock(int32_t timeout_ms)
{
    if (s_ctx.lvgl_mux == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTakeRecursive(s_ctx.lvgl_mux, timeout_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void esp_lv_adapter_unlock(void)
{
    if (s_ctx.lvgl_mux != nullptr) {
        xSemaphoreGiveRecursive(s_ctx.lvgl_mux);
    }
}

esp_err_t esp_lv_adapter_deinit(void)
{
    if (!s_ctx.initialized) {
        return ESP_OK;
    }

    if (s_ctx.lvgl_task_handle != nullptr) {
        vTaskDelete(s_ctx.lvgl_task_handle);
        s_ctx.lvgl_task_handle = nullptr;
    }

    if (s_ctx.indev != nullptr) {
        lv_indev_delete(s_ctx.indev);
        s_ctx.indev = nullptr;
    }

    if (s_ctx.display != nullptr) {
        lv_display_delete(s_ctx.display);
        s_ctx.display = nullptr;
    }

    if (s_ctx.lvgl_tick_timer != nullptr) {
        esp_timer_stop(s_ctx.lvgl_tick_timer);
        esp_timer_delete(s_ctx.lvgl_tick_timer);
        s_ctx.lvgl_tick_timer = nullptr;
    }

    if (s_ctx.buf_allocated) {
        if (s_ctx.buf1 != nullptr) {
            free(s_ctx.buf1);
        }
        if (s_ctx.buf2 != nullptr) {
            free(s_ctx.buf2);
        }
    }

    if (s_ctx.lvgl_mux != nullptr) {
        vSemaphoreDelete(s_ctx.lvgl_mux);
        s_ctx.lvgl_mux = nullptr;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    return ESP_OK;
}
