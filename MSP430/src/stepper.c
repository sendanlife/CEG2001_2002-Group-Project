#include "stepper.h"

#define STEP_DELAY_CYCLES 12000   // ~12 ms per step at 1 MHz (~80 steps/s)

#define STEPPER1_MASK (BIT2 | BIT3 | BIT4 | BIT5)   // P1.2 - P1.5
#define STEPPER2_MASK (BIT1 | BIT2 | BIT3 | BIT4)   // P6.1 - P6.4

const uint8_t step_sequence_1[4] = {
    BIT2 | BIT3,
    BIT3 | BIT4,
    BIT4 | BIT5,
    BIT5 | BIT2
};

const uint8_t step_sequence_2[4] = {
    BIT1 | BIT2,
    BIT2 | BIT3,
    BIT3 | BIT4,
    BIT4 | BIT1
};

void stepper_init(void) {
    // Stepper 1: plain GPIO outputs, initially off
    P1SEL &= ~STEPPER1_MASK;
    P1DIR |=  STEPPER1_MASK;
    P1OUT &= ~STEPPER1_MASK;

    // Stepper 2: plain GPIO outputs, initially off
    P6SEL &= ~STEPPER2_MASK;
    P6DIR |=  STEPPER2_MASK;
    P6OUT &= ~STEPPER2_MASK;
}

void stepper1Rotate(int steps) {
    int i;
    for (i = 0; i < steps; i++) {
        uint8_t idx = (uint8_t)(i & 0x03);
        P1OUT = (P1OUT & ~STEPPER1_MASK) | step_sequence_1[idx];
        __delay_cycles(STEP_DELAY_CYCLES);
    }
    P1OUT &= ~STEPPER1_MASK;   // de-energise coils
}

void stepper2Rotate(int steps) {
    int i;
    for (i = 0; i < steps; i++) {
        uint8_t idx = (uint8_t)(i & 0x03);
        P6OUT = (P6OUT & ~STEPPER2_MASK) | step_sequence_2[idx];
        __delay_cycles(STEP_DELAY_CYCLES);
    }
    P6OUT &= ~STEPPER2_MASK;
}