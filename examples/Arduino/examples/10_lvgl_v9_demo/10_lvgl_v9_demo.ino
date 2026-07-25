#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include <demos/lv_demos.h>

#include "lvgl_v9_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing board");

    Board *board = new Board();
    board->init();

    auto lcd = board->getLCD();
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        lcd->configFrameBufferNumber(LVGL_V9_PORT_FRAME_BUFFER_NUM);
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }

    assert(board->begin());

    Serial.println("Initializing LVGL v9");
    lvgl_v9_port_init(lcd, board->getTouch());

    Serial.println("Creating demo UI");
    lvgl_v9_port_lock(-1);
    lv_demo_widgets();
    lvgl_v9_port_unlock();
}

void loop()
{
    Serial.println("IDLE loop");
    delay(1000);
}
