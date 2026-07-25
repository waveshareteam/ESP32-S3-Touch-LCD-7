// Define 485 communication pins
#define RS485_RX_PIN  15
#define RS485_TX_PIN  16

// Redefine serial port name
#define RS485 Serial1

void setup() {
  // Initialize 485 device
  RS485.begin(115200, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  while (!RS485) {
    delay(10); // Wait for initialization to succeed
  }
}

void loop() {
  // Waiting for 485 data, cannot exceed 120 characters
  if (RS485.available()) {
    // Send the received data back
    RS485.write(RS485.read());
  }
}
