#include <msp430.h>
#include "irSensor.h"

#define IR1_BIT BIT0     // P2.0 -> stepper 1
#define IR2_BIT BIT2     // P2.2 -> stepper 2

volatile unsigned char pill_detected_flag1 = 0;
volatile unsigned char pill_detected_flag2 = 0;

// Idle level learned at the start of each dispense: 1 = HIGH, 0 = LOW.
// A pill is any change away from that level.
static unsigned char learned[2]  = {0, 0};
static unsigned char idleHigh[2] = {1, 1};

void IRSensor_Init(void) {
    P2DIR &= ~(IR1_BIT | IR2_BIT);   // inputs
    P2REN |=  (IR1_BIT | IR2_BIT);   // resistors on
    P2OUT |=  (IR1_BIT | IR2_BIT);   // pull-up
    P2IFG &= ~(IR1_BIT | IR2_BIT);
    P2IE  &= ~(IR1_BIT | IR2_BIT);   // armed later, per motor
}

void IRSensor_Arm(unsigned char motor) {
    unsigned char i   = (motor == 2) ? 1 : 0;
    unsigned char bit = (motor == 2) ? IR2_BIT : IR1_BIT;

    // learn the idle level only once per dispense (main re-arms after each pill)
    if (!learned[i]) {
        idleHigh[i] = (P2IN & bit) ? 1 : 0;
        learned[i] = 1;
    }

    // idle HIGH -> a pill pulls it LOW (falling edge)
    // idle LOW  -> a pill pushes it HIGH (rising edge)
    if (idleHigh[i]) {
        P2IES |= bit;
    } else {
        P2IES &= ~bit;
    }

    if (i) {
        pill_detected_flag2 = 0;
    } else {
        pill_detected_flag1 = 0;
    }

    P2IFG &= ~bit;      // clear AFTER changing the edge
    P2IE  |=  bit;
}

void IRSensor_Disarm(unsigned char motor) {
    unsigned char i   = (motor == 2) ? 1 : 0;
    unsigned char bit = (motor == 2) ? IR2_BIT : IR1_BIT;

    P2IE  &= ~bit;
    P2IFG &= ~bit;
    learned[i] = 0;     // learn the idle level again at the next dispense
}

unsigned char IRSensor_PillDetected(unsigned char motor) {
    return (motor == 2) ? pill_detected_flag2 : pill_detected_flag1;
}

void __attribute__((interrupt(PORT2_VECTOR))) Port_2_ISR(void) {
    if (P2IFG & IR1_BIT) {
        pill_detected_flag1 = 1;
        P2IE  &= ~IR1_BIT;       // lock out until main re-arms (debounce)
        P2IFG &= ~IR1_BIT;
    }
    if (P2IFG & IR2_BIT) {
        pill_detected_flag2 = 1;
        P2IE  &= ~IR2_BIT;       // lock out until main re-arms (debounce)
        P2IFG &= ~IR2_BIT;
    }
}