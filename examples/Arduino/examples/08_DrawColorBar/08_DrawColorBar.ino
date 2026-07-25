#include "waveshare_lcd_port.h"

void setup()
{
    Serial.begin(115200); // Initialize serial communication at 115200 baud rate
    Serial.println("RGB LCD example start"); // Print start message for RGB LCD example
    waveshare_lcd_init(); // Initialize the RGB LCD
    Serial.println("RGB LCD example end"); // Print end message for RGB LCD example
}

void loop()
{
    delay(1000); // Wait for 1 second
    Serial.println("IDLE loop"); // Print idle loop message

}
