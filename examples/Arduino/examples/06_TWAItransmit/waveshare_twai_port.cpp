#include "waveshare_twai_port.h"

unsigned long previousMillis = 0;  // Will store the last time a message was sent

// Send message
static void send_message() {
  // Configure message to transmit
  twai_message_t message;
  message.identifier = 0x0F6; // Set message identifier
  message.data_length_code = 8; // Set data length
  for (int i = 0; i < message.data_length_code; i++) {
    message.data[i] = i; // Populate message data
  }
  // Queue message for transmission 
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    printf("Message queued for transmission\n"); // Print success message
  } else {
    printf("Failed to queue message for transmission\n"); // Print failure message
  }
  memset(message.data, 0, sizeof(message.data)); // Clear the entire array
}

bool waveshare_twai_init()
{ 
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();  // Set 50Kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Accept all messages

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("Failed to install driver"); // Print error message
        return false; // Return false if driver installation fails
    }
    Serial.println("Driver installed"); // Print success message

    // Start TWAI driver
    if (twai_start() != ESP_OK) {
        Serial.println("Failed to start driver"); // Print error message
        return false; // Return false if starting the driver fails
    }
    Serial.println("Driver started"); // Print success message

    // Reconfigure alerts to detect TX alerts and Bus-Off errors
    uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) != ESP_OK) {
        Serial.println("Failed to reconfigure alerts"); // Print error message
        return false; // Return false if alert reconfiguration fails
    }
    Serial.println("CAN Alerts reconfigured"); // Print success message

    // TWAI driver is now successfully installed and started
    return true; // Return true on success
}

void waveshare_twai_transmit()
{
    // Check if alert happened
    uint32_t alerts_triggered;
    twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS)); // Read triggered alerts
    twai_status_info_t twaistatus; // Create status info structure
    twai_get_status_info(&twaistatus); // Get status information

    // Handle alerts
    if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
      Serial.println("Alert: TWAI controller has become error passive."); // Print passive error alert
    }
    if (alerts_triggered & TWAI_ALERT_BUS_ERROR) {
      Serial.println("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus."); // Print bus error alert
      Serial.printf("Bus error count: %d\n", twaistatus.bus_error_count); // Print bus error count
    }
    if (alerts_triggered & TWAI_ALERT_TX_FAILED) {
      Serial.println("Alert: The Transmission failed."); // Print transmission failure alert
      Serial.printf("TX buffered: %d\t", twaistatus.msgs_to_tx); // Print buffered TX messages
      Serial.printf("TX error: %d\t", twaistatus.tx_error_counter); // Print TX error count
      Serial.printf("TX failed: %d\n", twaistatus.tx_failed_count); // Print failed TX count
    }
    if (alerts_triggered & TWAI_ALERT_TX_SUCCESS) {
      Serial.println("Alert: The Transmission was successful."); // Print successful transmission alert
      Serial.printf("TX buffered: %d\t", twaistatus.msgs_to_tx); // Print buffered TX messages
    }

    // Send message
    unsigned long currentMillis = millis(); // Get current time in milliseconds
    if (currentMillis - previousMillis >= TRANSMIT_RATE_MS) { // Check if it's time to send a message
      previousMillis = currentMillis; // Update last send time
      send_message(); // Call function to send message
    }
}
