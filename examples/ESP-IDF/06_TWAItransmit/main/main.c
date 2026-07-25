/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*
 * The following example demonstrates a master node in a TWAI network. The master
 * node is responsible for initiating and stopping the transfer of data messages.
 * The example will execute multiple iterations, with each iteration the master
 * node will do the following:
 * 1) Start the TWAI driver
 * 2) Repeatedly send ping messages until a ping response from slave is received
 * 3) Send start command to slave and receive data messages from slave
 * 4) Send stop command to slave and wait for stop response from slave
 * 5) Stop the TWAI driver
 */
#include "waveshare_twai_port.h" // Include the Waveshare TWAI port library 

void app_main(void) // Main application entry point 
{
    waveshare_twai_init(); // Initialize the Waveshare TWAI module 
    while (1)              // Infinite loop 
    {
        waveshare_twai_transmit(); // Transmit data using Waveshare TWAI 
    }
}
