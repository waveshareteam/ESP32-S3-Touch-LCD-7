#include "waveshare_twai_port.h"

static bool driver_installed = false; // Flag to check if the driver is installed
esp_expander::CH422G *expander = NULL;

void setup() {
  // Start Serial 
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  
  Serial.println("Initialize IO expander");
  /* Initialize IO expander */
  expander = new esp_expander::CH422G(EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, EXAMPLE_I2C_ADDR);
  expander->init();
  expander->begin();

  Serial.println("Set the IO0-7 pin to output mode.");
  expander->enableAllIO_Output();
  expander->multiDigitalWrite(TP_RST | LCD_RST | USB_SEL, HIGH);
  expander->digitalWrite(LCD_BL , LOW);
  
  driver_installed = waveshare_twai_init(); // Initialize the TWAI driver and store the result
}

void loop() {
  if (!driver_installed) {
    // Driver not installed 
    delay(1000); // Wait for 1 second
    return; // Exit the loop if the driver is not installed
  }
  waveshare_twai_transmit(); // Call the transmit function if the driver is installed
}
