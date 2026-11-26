#include <msp430.h>
#include "stepper.h"

#define STEP_DELAY_CYCLES 12000  

#define STEPPER1_MASK (BIT2 | BIT3 | BIT4 | BIT5)  
#define STEPPER2_MASK (BIT1 | BIT2 | BIT3 | BIT4)   

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

static uint8_t phase1 = 0;
static uint8_t phase2 = 0;

void stepper_init(void) {
    P1SEL &= ~STEPPER1_MASK;
    P1DIR |=  STEPPER1_MASK;
    P1OUT &= ~STEPPER1_MASK;

    P6SEL &= ~STEPPER2_MASK;
    P6DIR |=  STEPPER2_MASK;
    P6OUT &= ~STEPPER2_MASK;
}

void stepper1Step(void) {
    P1OUT = (P1OUT & ~STEPPER1_MASK) | step_sequence_1[phase1];
    phase1 = (phase1 + 1) & 0x03;
    __delay_cycles(STEP_DELAY_CYCLES);
}

void stepper2Step(void) {
    P6OUT = (P6OUT & ~STEPPER2_MASK) | step_sequence_2[phase2];
    phase2 = (phase2 + 1) & 0x03;
    __delay_cycles(STEP_DELAY_CYCLES);
}

void stepper1Release(void) { P1OUT &= ~STEPPER1_MASK; }
void stepper2Release(void) { P6OUT &= ~STEPPER2_MASK; }

void stepper1Rotate(int steps) {
    int i;
    for (i = 0; i < steps; i++) {
        stepper1Step();
    }
    stepper1Release();
}

void stepper2Rotate(int steps) {
    int i;
    for (i = 0; i < steps; i++) {
        stepper2Step();
    }
    stepper2Release();
}