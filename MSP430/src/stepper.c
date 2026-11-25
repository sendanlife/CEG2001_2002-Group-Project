#include <msp430.h>
#include "stepper.h"

#define STEPPER1_MASK (BIT2 | BIT3 | BIT4 | BIT5)  // P1.2 - P1.5 (Small pills)
#define STEPPER2_MASK (BIT1 | BIT2 | BIT3 | BIT4)  // P6.1 - P6.4 (Large pills)

// 2-phase full-step sequence for higher torque on 28BYJ-48
static const unsigned char stepSeq[4] = { 0x03, 0x06, 0x0C, 0x09 };

void stepper_init(void) {
    // Stepper 1 (Small): P1.2 - P1.5
    P1SEL &= ~STEPPER1_MASK;
    P1DIR |= STEPPER1_MASK;
    P1OUT &= ~STEPPER1_MASK;

    // Stepper 2 (Large): P6.1 - P6.4 (leaves P6.0 ADC untouched)
    P6SEL &= ~STEPPER2_MASK;
    P6DIR |= STEPPER2_MASK;
    P6OUT &= ~STEPPER2_MASK;
}

void stepper_rotate(unsigned char motor, unsigned int steps) {
    unsigned int i;
    for (i = 0; i < steps; i++) {
        unsigned char pattern = stepSeq[i & 0x03];

        if (motor == STEPPER_SMALL) {
            P6OUT = (P6OUT & ~STEPPER2_MASK) | (unsigned char)(pattern << 1);
        } else {
            P1OUT = (P1OUT & ~STEPPER1_MASK) | (unsigned char)(pattern << 2);
        }

        __delay_cycles(10000);  // ~3 ms per step at 1 MHz; adjust if your clock differs
    }

    // De-energize coils to prevent overheating and save power
    if (motor == STEPPER_LARGE) {
        P6OUT &= ~STEPPER2_MASK;
    } else {
        P1OUT &= ~STEPPER1_MASK;
    }
}