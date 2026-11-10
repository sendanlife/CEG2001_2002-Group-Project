#include "stepper.h"
#include <Arduino.h>

// --- Pin Definitions ---
#define STEP_PIN 18
#define DIR_PIN  19
#define EN_PIN   21

// --- Configuration ---
#define STEPS_TO_DISPENSE 50 

void Stepper_Init(void) {
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);

    // Disable motor driver initially to save power
    digitalWrite(EN_PIN, HIGH); 
    digitalWrite(STEP_PIN, LOW);
}

void Stepper_Dispense(void) {
    digitalWrite(EN_PIN, LOW);  // Enable motor (Assuming LOW is active)
    digitalWrite(DIR_PIN, HIGH); // Set direction
    
    // Step the motor
    for (int i = 0; i < STEPS_TO_DISPENSE; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(1000); 
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
    
    digitalWrite(EN_PIN, HIGH); // Disable motor to prevent overheating
}