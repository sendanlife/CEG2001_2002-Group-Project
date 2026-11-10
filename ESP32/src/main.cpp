#include <Arduino.h>
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
}