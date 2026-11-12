#include <Arduino.h>
#include "esp32uart.h"
#include "nfc.h"

/*
 * Summary: Main application file for the ESP32 subsystem. Currently 
 * configured strictly to test and run the bidirectional UART bridge 
 * to the MSP430 Launchpad.
 * Code Structure: setup(), loop()
 * Author(s): [Insert Team Member 1]
 * Version: 1.0
 */

// Define the hardware UART pins for ESP32 to MSP430 communication
#define MSP430_RX_PIN 16 
#define MSP430_TX_PIN 17

void setup() {
    // 1. Initialize USB Serial Monitor for VS Code debugging
    Serial.begin(115200);

    // 2. Initialize UART2 to talk to the MSP430 at 9600 baud
    Serial2.begin(9600, SERIAL_8N1, MSP430_RX_PIN, MSP430_TX_PIN);

    // 3. Initialize the NFC module using your custom function
    init_nfc();
    
    Serial.println("ESP32 Booted. Waiting for authorized card tap...");
}

void loop() {
    // Check if the correct card has been tapped
    if (check_nfc_card()) {
        
        Serial.println("Authorized Card Detected! Dispensing...");

        // Send a single character 'D' to trigger the MSP430 FSM
        Serial2.print('D'); 
        
        // 3-second delay to prevent spamming the UART buffer if the card is held down
        delay(3000); 
        
        Serial.println("Ready for next tap...");
    }
    
    // Background delay to keep the ESP32 watchdog timer stable
    delay(50);
}