# ESP32-S3-Touch-LCD-7 firmware

[中文](README_ZH.md)

| File | Purpose | Flash offset |
| --- | --- | --- |
| [ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin](ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin) | Factory test firmware for the touch model | 0x0 |
| [DrawColorBar.bin](DrawColorBar.bin) | LCD color bar example | 0x0 |
| [I2C_Test.bin](I2C_Test.bin) | I2C test | 0x0 |
| [lvgl_Porting.bin](lvgl_Porting.bin) | LVGL example for the touch model | 0x0 |
| [lvgl_Porting_No_Touch.bin](lvgl_Porting_No_Touch.bin) | LVGL example for the non-touch model | 0x0 |
| [RS485_Test.bin](RS485_Test.bin) | RS485 test | 0x0 |
| [SD_Test.bin](SD_Test.bin) | SD card test | 0x0 |
| [Sensor_AD.bin](Sensor_AD.bin) | Analog sensor example | 0x0 |
| [TWAIreceive.bin](TWAIreceive.bin) | TWAI receive example | 0x0 |
| [TWAItransmit.bin](TWAItransmit.bin) | TWAI transmit example | 0x0 |
| [UART_Test.bin](UART_Test.bin) | UART test | 0x0 |

Change to this directory and flash the factory firmware with esptool:

    esptool --chip esp32s3 write-flash 0x0 ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin
