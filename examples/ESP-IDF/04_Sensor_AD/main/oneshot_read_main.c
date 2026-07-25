/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "EXAMPLE";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
#define EXAMPLE_ADC1_CHAN5 ADC_CHANNEL_5  // ADC channel 5, which is GPIO6
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_12  // ADC attenuation level

static int adc_raw[1][10];  // Array to store raw ADC values
static int voltage[1][10];   // Array to store calculated voltage values
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;  // Handle for ADC unit 1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,  // Initialize ADC unit 1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));  // Create new ADC unit

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // Default bit width for ADC
        .atten = EXAMPLE_ADC_ATTEN,  // Set the attenuation level
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN5, &config));  // Configure ADC channel
    // ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config)); // Uncomment to configure another channel

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;  // Handle for ADC calibration
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN5, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);

    while (1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN5, &adc_raw[0][0]));  // Read raw data from ADC
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5, adc_raw[0][0]);  // Log raw data
        if (do_calibration1_chan0)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));  // Convert raw data to voltage
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN5, voltage[0][0]);  // Log calibrated voltage
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    }

    // Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));  // Delete ADC unit
    if (do_calibration1_chan0)
    {
        example_adc_calibration_deinit(adc1_cali_chan0_handle);  // Deinitialize calibration handle
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;  // Handle for calibration
    esp_err_t ret = ESP_FAIL;  // Return value
    bool calibrated = false;  // Calibration status

    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");  // Log calibration scheme version
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,  // Set ADC unit ID
            .chan = channel,  // Set ADC channel
            .atten = atten,   // Set attenuation
            .bitwidth = ADC_BITWIDTH_DEFAULT,  // Set default bit width
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);  // Create calibration scheme
        if (ret == ESP_OK)
        {
            calibrated = true;  // Set calibrated status to true
        }
    }

    *out_handle = handle;  // Output handle
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");  // Log successful calibration
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");  // Log warning for unsupported calibration
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");  // Log error for invalid arguments or memory issues
    }

    return calibrated;  // Return calibration status
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");  // Log deregistration of calibration scheme
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));  // Delete calibration scheme
}
