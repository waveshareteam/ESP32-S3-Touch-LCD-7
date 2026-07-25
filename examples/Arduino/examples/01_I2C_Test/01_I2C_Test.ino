#include "Wire.h"

void setup() {
  Serial.begin(115200); // Initialize USB printing function, baud rate is 115200
  Wire.begin(8, 9); // Initialize the I2C device and set SDA to 8 and SCL to 9
}

void loop() {
  byte error, address; // Store error content, address value
  int nDevices = 0; // Number of recording devices

  printf("Scanning for I2C devices ...\r\n");
  for (address = 0x01; address < 0x7f; address++) { // Start scanning all devices from 0x01 to 0x7f
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) { // Determine whether the device exists, and if so, output the address
      printf("I2C device found at address 0x%02X\n", address);
      nDevices++;
    } else if (error != 2) {
      printf("Error %d at address 0x%02X\n", error, address);
    }
  }
  if (nDevices == 0) { // Determine the number of devices. If it is 0, there is no device.
    printf("No I2C devices found.\r\n");
  }
  delay(5000); // Scan every 5 seconds
}
