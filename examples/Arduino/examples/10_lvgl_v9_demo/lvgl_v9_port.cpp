#include "lvgl_v9_port.h"

#include "esp_timer.h"
#include <limits.h>

using namespace esp_panel::drivers;

static SemaphoreHandle_t s_lvgl_mux = nullptr;
static TaskHandle_t s_lvgl_task_handle = nullptr;
static esp_timer_handle_t s_lvgl_tick_timer = nullptr;
static LCD *s_lcd = nullptr;
static Touch *s_touch = nullptr;
static lv_display_t *s_display = nullptr;
static lv_indev_t *s_indev = nullptr;
static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;
static bool s_use_direct_mode = false;

IRAM_ATTR static bool lcd_vsync_cb(void *user_data)
{
    BaseType_t need_yield = pdFALSE;
    TaskHandle_t task_handle = static_cast<TaskHandle_t>(user_data);
    xTaskNotifyFromISR(task_handle, ULONG_MAX, eNoAction, &need_yield);
    return (need_yield == pdTRUE);
}

static void lvgl_tick_cb(void *arg)
{
    LV_UNUSED(arg);
    lv_tick_inc(LVGL_V9_PORT_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg)
{
    LV_UNUSED(arg);

    uint32_t delay_ms = LVGL_V9_PORT_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_v9_port_lock(-1)) {
            delay_ms = lv_timer_handler();
            lvgl_v9_port_unlock();
        }

        if (delay_ms < LVGL_V9_PORT_TASK_MIN_DELAY_MS) {
            delay_ms = LVGL_V9_PORT_TASK_MIN_DELAY_MS;
        } else if (delay_ms > LVGL_V9_PORT_TASK_MAX_DELAY_MS) {
            delay_ms = LVGL_V9_PORT_TASK_MAX_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (s_use_direct_mode) {
        if (lv_display_flush_is_last(disp)) {
            s_lcd->switchFrameBufferTo(px_map);
            ulTaskNotifyValueClear(nullptr, ULONG_MAX);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
        lv_display_flush_ready(disp);
        return;
    }

    const int32_t width = area->x2 - area->x1 + 1;
    const int32_t height = area->y2 - area->y1 + 1;
    s_lcd->drawBitmap(area->x1, area->y1, width, height, px_map);
    lv_display_flush_ready(disp);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    LV_UNUSED(indev);

    if (s_touch == nullptr) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    TouchPoint point;
    const int count = s_touch->readPoints(&point, 1, 0);
    if (count > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool lvgl_v9_port_init(LCD *lcd, Touch *tp)
{
    if (lcd == nullptr) {
        return false;
    }

    s_lcd = lcd;
    s_touch = tp;

    lv_init();

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &s_lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, LVGL_V9_PORT_TICK_PERIOD_MS * 1000));

    const uint32_t width = s_lcd->getFrameWidth();
    const uint32_t height = s_lcd->getFrameHeight();
    const auto bus_type = s_lcd->getBus()->getBasicAttributes().type;
    uint32_t buffer_pixels = width * LVGL_V9_PORT_BUFFER_LINES;
    uint32_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    s_use_direct_mode = (bus_type == ESP_PANEL_BUS_TYPE_RGB) &&
                        (s_lcd->getFrameBufferByIndex(0) != nullptr) &&
                        (s_lcd->getFrameBufferByIndex(1) != nullptr);

    if (s_use_direct_mode) {
        buffer_pixels = width * height;
        buffer_bytes = buffer_pixels * sizeof(lv_color_t);
        s_buf1 = static_cast<lv_color_t *>(s_lcd->getFrameBufferByIndex(0));
        s_buf2 = static_cast<lv_color_t *>(s_lcd->getFrameBufferByIndex(1));
    } else {
        s_buf1 = static_cast<lv_color_t *>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        s_buf2 = static_cast<lv_color_t *>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }

    assert(s_buf1 != nullptr);
    assert(s_buf2 != nullptr);

    s_display = lv_display_create(width, height);
    assert(s_display != nullptr);
    lv_display_set_flush_cb(s_display, display_flush_cb);
    lv_display_set_buffers(
        s_display,
        s_buf1,
        s_buf2,
        buffer_bytes,
        s_use_direct_mode ? LV_DISPLAY_RENDER_MODE_DIRECT : LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(s_display);

    if (s_touch != nullptr) {
        s_indev = lv_indev_create();
        assert(s_indev != nullptr);
        lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(s_indev, touch_read_cb);
        lv_indev_set_display(s_indev, s_display);
    }

    s_lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(s_lvgl_mux != nullptr);

    BaseType_t ret = xTaskCreatePinnedToCore(
        lvgl_port_task,
        "lvgl",
        LVGL_V9_PORT_TASK_STACK_SIZE,
        nullptr,
        LVGL_V9_PORT_TASK_PRIORITY,
        &s_lvgl_task_handle,
        ARDUINO_RUNNING_CORE);
    if (ret != pdPASS) {
        return false;
    }

    if (s_use_direct_mode) {
        s_lcd->attachRefreshFinishCallback(lcd_vsync_cb, s_lvgl_task_handle);
    }

    return true;
}

bool lvgl_v9_port_lock(int timeout_ms)
{
    if (s_lvgl_mux == nullptr) {
        return false;
    }

    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mux, timeout_ticks) == pdTRUE;
}

bool lvgl_v9_port_unlock(void)
{
    if (s_lvgl_mux == nullptr) {
        return false;
    }

    xSemaphoreGiveRecursive(s_lvgl_mux);
    return true;
}
