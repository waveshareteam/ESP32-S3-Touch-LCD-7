/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sd_card.h"

void app_main(void)
{
    // Initialize SD card 
    if(waveshare_sd_card_init() == ESP_OK)
    {
        // Test SD card functionality 
        waveshare_sd_card_test();
    }

    
}

