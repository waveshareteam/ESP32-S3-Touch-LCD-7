#include "waveshare_twai_port.h" // Include the Waveshare TWAI port library

static bool driver_installed = false; // Driver installation status
unsigned long previousMillis = 0;     // Will store last time a message was sent

// TWAI timing configuration for 50 Kbits
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();
// TWAI filter configuration to accept all messages
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
// General configuration for TWAI
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Function to send a message
static void send_message()
{
    // Configure message to transmit
    twai_message_t message;       // Message structure
    message.identifier = 0x0F6;   // Message identifier
    message.data_length_code = 8; // Data length code
    for (int i = 0; i < 8; i++)
    {
        message.data[i] = i; // Set message data
    }

    // Queue message for transmission
    if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
    {
        printf("Message queued for transmission\n"); // Success message
    }
    else
    {
        printf("Failed to queue message for transmission\n"); // Failure message
    }
}

// Function to initialize Waveshare TWAI
esp_err_t waveshare_twai_init()
{
    ESP_ERROR_CHECK(i2c_master_init());

    // When USB_SEL is HIGH, it enables FSUSB42UMX chip and gpio19,gpio20 wired CAN_TX CAN_RX, and then dont use USB Function
    uint8_t write_buf = 0x01;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    write_buf = 0x20;
    i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        ESP_LOGI(EXAMPLE_TAG, "Driver installed"); // Log driver installed
    }
    else
    {
        ESP_LOGI(EXAMPLE_TAG, "Failed to install driver"); // Log installation failure
        return ESP_FAIL;                                   // Return failure status
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        ESP_LOGI(EXAMPLE_TAG, "Driver started"); // Log driver started
    }
    else
    {
        ESP_LOGI(EXAMPLE_TAG, "Failed to start driver"); // Log start failure
        return ESP_FAIL;                                 // Return failure status
    }

    // Reconfigure alerts to detect TX alerts and Bus-Off errors
    uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
        ESP_LOGI(EXAMPLE_TAG, "CAN Alerts reconfigured"); // Log alerts reconfigured
    }
    else
    {
        ESP_LOGI(EXAMPLE_TAG, "Failed to reconfigure alerts"); // Log reconfiguration failure
        return ESP_FAIL;                                       // Return failure status
    }
    // TWAI driver is now successfully installed and started
    driver_installed = true; // Update driver installation status
    return ESP_OK;           // Return success status
}

// Function to transmit messages
esp_err_t waveshare_twai_transmit()
{

    if (!driver_installed)
    {
        // Driver not installed
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before retrying
        return ESP_FAIL;                 // Return failure status
    }
    // Check if alert happened
    uint32_t alerts_triggered;                                           // Variable to store triggered alerts
    twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS)); // Read alerts
    twai_status_info_t twaistatus;                                       // Variable to store TWAI status information
    twai_get_status_info(&twaistatus);                                   // Get TWAI status information

    // Handle alerts
    if (alerts_triggered & TWAI_ALERT_ERR_PASS)
    {
        ESP_LOGI(EXAMPLE_TAG, "Alert: TWAI controller has become error passive."); // Log error passive alert
    }
    if (alerts_triggered & TWAI_ALERT_BUS_ERROR)
    {
        ESP_LOGI(EXAMPLE_TAG, "Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus."); // Log bus error alert
        ESP_LOGI(EXAMPLE_TAG, "Bus error count: %" PRIu32, twaistatus.bus_error_count);                // Log bus error count
    }
    if (alerts_triggered & TWAI_ALERT_TX_FAILED)
    {
        ESP_LOGI(EXAMPLE_TAG, "Alert: The Transmission failed.");                 // Log transmission failure alert
        ESP_LOGI(EXAMPLE_TAG, "TX buffered: %" PRIu32, twaistatus.msgs_to_tx);    // Log buffered messages count
        ESP_LOGI(EXAMPLE_TAG, "TX error: %" PRIu32, twaistatus.tx_error_counter); // Log transmission error count
        ESP_LOGI(EXAMPLE_TAG, "TX failed: %" PRIu32, twaistatus.tx_failed_count); // Log failed transmission count
    }
    if (alerts_triggered & TWAI_ALERT_TX_SUCCESS)
    {
        ESP_LOGI(EXAMPLE_TAG, "Alert: The Transmission was successful.");      // Log transmission success alert
        ESP_LOGI(EXAMPLE_TAG, "TX buffered: %" PRIu32, twaistatus.msgs_to_tx); // Log buffered messages count
    }

    // Send message
    unsigned long currentMillis = esp_timer_get_time() / 1000; // Get current time in milliseconds
    if (currentMillis - previousMillis >= TRANSMIT_RATE_MS)
    {                                   // Check if it's time to send the message
        previousMillis = currentMillis; // Update last send time
        send_message();                 // Call send message function
    }
    return ESP_OK; // Return success status
}
