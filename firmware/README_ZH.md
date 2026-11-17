# ESP32-S3-Touch-LCD-7 固件

[English](README.md)

| 文件 | 用途 | 烧录偏移 |
| --- | --- | --- |
| [ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin](ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin) | 触屏版出厂测试固件 | 0x0 |
| [DrawColorBar.bin](DrawColorBar.bin) | LCD 彩条示例 | 0x0 |
| [I2C_Test.bin](I2C_Test.bin) | I2C 测试 | 0x0 |
| [lvgl_Porting.bin](lvgl_Porting.bin) | 触屏版 LVGL 示例 | 0x0 |
| [lvgl_Porting_No_Touch.bin](lvgl_Porting_No_Touch.bin) | 非触屏版 LVGL 示例 | 0x0 |
| [RS485_Test.bin](RS485_Test.bin) | RS485 测试 | 0x0 |
| [SD_Test.bin](SD_Test.bin) | SD 卡测试 | 0x0 |
| [Sensor_AD.bin](Sensor_AD.bin) | 模拟传感器示例 | 0x0 |
| [TWAIreceive.bin](TWAIreceive.bin) | TWAI 接收示例 | 0x0 |
| [TWAItransmit.bin](TWAItransmit.bin) | TWAI 发送示例 | 0x0 |
| [UART_Test.bin](UART_Test.bin) | UART 测试 | 0x0 |

进入此目录后，使用 esptool 烧录出厂固件。

    esptool --chip esp32s3 write-flash 0x0 ESP32-S3-Touch-LCD-7_FactoryOnly_20260625.bin
