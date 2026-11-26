#include <msp430.h>
#include "irSensor.h"

#define IR1_BIT BIT0     // P2.0 -> stepper 1
#define IR2_BIT BIT2     // P2.2 -> stepper 2

volatile unsigned char pill_detected_flag1 = 0;
volatile unsigned char pill_detected_flag2 = 0;

void IRSensor_Init(void) {
    P2DIR &= ~(IR1_BIT | IR2_BIT);   // inputs
    P2REN |=  (IR1_BIT | IR2_BIT);   // pull resistors on
    P2OUT |=  (IR1_BIT | IR2_BIT);   // pull-up
    P2IES |=  (IR1_BIT | IR2_BIT);   // trigger on falling edge
    P2IFG &= ~(IR1_BIT | IR2_BIT);
    P2IE  &= ~(IR1_BIT | IR2_BIT);   // armed later, per motor
}

void IRSensor_Arm(unsigned char motor) {
    if (motor == 2) {
        pill_detected_flag2 = 0;
        P2IFG &= ~IR2_BIT;
        P2IE  |=  IR2_BIT;
    } else {
        pill_detected_flag1 = 0;
        P2IFG &= ~IR1_BIT;
        P2IE  |=  IR1_BIT;
    }
}

void IRSensor_Disarm(unsigned char motor) {
    if (motor == 2) {
        P2IE  &= ~IR2_BIT;
        P2IFG &= ~IR2_BIT;
    } else {
        P2IE  &= ~IR1_BIT;
        P2IFG &= ~IR1_BIT;
    }
}

unsigned char IRSensor_PillDetected(unsigned char motor) {
    return (motor == 2) ? pill_detected_flag2 : pill_detected_flag1;
}

void __attribute__((interrupt(PORT2_VECTOR))) Port_2_ISR(void) {
    if (P2IFG & IR1_BIT) {
        pill_detected_flag1 = 1;
        P2IFG &= ~IR1_BIT;
    }
    if (P2IFG & IR2_BIT) {
        pill_detected_flag2 = 1;
        P2IFG &= ~IR2_BIT;
    }
}