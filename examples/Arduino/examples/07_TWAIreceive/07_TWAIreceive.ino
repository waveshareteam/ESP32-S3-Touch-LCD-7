#include "waveshare_twai_port.h"

static bool driver_installed = false; // Flag to check if the driver is installed
esp_expander::CH422G *expander = NULL;

void setup() {
  // Start Serial:
  Serial.begin(115200);// Flag to check if the driver is installed

  Serial.println("Initialize IO expander");
  /* Initialize IO expander */
  expander = new esp_expander::CH422G(EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, EXAMPLE_I2C_ADDR);
  expander->init();
  expander->begin();

  Serial.println("Set the IO0-7 pin to output mode.");
  expander->enableAllIO_Output();
  expander->multiDigitalWrite(TP_RST | LCD_RST | USB_SEL, HIGH);
  expander->digitalWrite(LCD_BL , LOW);
  
  driver_installed = waveshare_twai_init(); // Flag to check if the driver is installed
}

void loop() {
  if (!driver_installed) {
    // Driver not installed
    delay(1000);
    return;
  }
  waveshare_twai_receive();// Call the receive function if the driver is installed
}
