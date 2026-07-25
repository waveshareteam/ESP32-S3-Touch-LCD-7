// Redefine serial port name
#define UART Serial

void setup() {
  // Initialize 485 device
  UART.begin(115200);
  while (!UART) {
    delay(10); // Wait for initialization to succeed
  }
}

void loop() {
  // Waiting for 485 data, cannot exceed 120 characters
  if (UART.available()) {
    // Send the received data back
    UART.write(UART.read());
  }
}
