#include "stepper.h"

// 4-step sequence for high torque on the 28BYJ-48 Stepper
const uint8_t step_sequence[4] = {BIT2 | BIT3, BIT3 | BIT4, BIT4 | BIT5, BIT5 | BIT2};

void stepper_init(void) {
    // Configure Stepper Motor Pins (P1.2, P1.3, P1.4, P1.5) as Outputs
    P1DIR |= BIT2 | BIT3 | BIT4 | BIT5;
    P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5); // Start with motor off
}

void stepper_rotate(int steps) {
    int i;
    for (i = 0; i < steps; i++) {
        P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5); // Clear motor pins
        P1OUT |= step_sequence[i % 4];         // Energize next coils
        __delay_cycles(20000);                 // 20ms delay gives motor time to move
    }
    // Turn all coils off when done so the motor doesn't overheat
    P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5);     
}