#include <Arduino.h>
<<<<<<< HEAD
#include "loadCell.h"
#include "stepMonitor.h"
#include "led.h"

// Business Logic Constants
const float PILL_THRESHOLD = 1.0; // Grams

void setup() {
    Serial.begin(115200);
    
    // Initialize all independent modules
    LoadCell_Init(4, 5); // DOUT, SCK
    Stepper_Init();
    LED_Init();
    
    LoadCell_Tare();
    Serial.println("MediTrack System Ready.");
}

void loop() {
    // 1. Get data from the sensor module
    float current_weight = LoadCell_GetWeightGrams();
    
    // 2. Apply your conditions
    if (current_weight > PILL_THRESHOLD) {
        // --- PILLS DETECTED ---
        LED_TurnOff();       // Ensure red LED is off
        Stepper_Dispense();  // Activate stepper motor
        
        Serial.println("Pills detected. Dispensing...");
        
        // Wait before checking again so the motor doesn't spin infinitely
        delay(5000); 
        
    } else {
        // --- NO PILLS DETECTED ---
        LED_TurnOn();        // Light up red LED
        // (We do nothing to the stepper, so it stays off)
        
        Serial.println("Storage empty. LED ON.");
    }
    
    delay(500); // Loop delay
=======
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
>>>>>>> 3816f91096e7de8e90fce94fbe42a159e886a8ef
}