#include <Arduino.h>
#include "esp32uart.h"

/*
 * Summary: Main application file for the ESP32 subsystem. Currently 
 * configured strictly to test and run the bidirectional UART bridge 
 * to the MSP430 Launchpad.
 * Code Structure: setup(), loop()
 * Author(s): [Insert Team Member 1]
 * Version: 1.0
 */

void setup() {
    // Initialize USB debugging serial port (Serial0)
    Serial.begin(115200);
    delay(1000); // Give the serial monitor a second to connect

    // Initialize UART bridge to MSP430 on Serial2
    esp32_uart_init();
    
    Serial.println("ESP32 Main: UART Bridge Ready.");
    Serial.println("Type 'AUTH' in the Serial Monitor to send the wake-up flag to the MSP430.");
}

void loop() {
    // 1. Listen for incoming messages from the MSP430 (e.g., "DROP_SUCCESS")
    String incoming_msg = esp32_check_messages();
    
    if (incoming_msg.length() > 0) {
        Serial.print("Received from MSP430: ");
        Serial.println(incoming_msg);
    }

    // 2. Listen for manual input from your computer to test the transmission
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "AUTH") {
            Serial.println("Simulating valid NFC tap...");
            esp32_send_authorization();
        }
    }
    
    delay(50); // Brief delay for loop stability
}