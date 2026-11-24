#include <msp430.h>
#include "irSensor.h"

volatile unsigned char pill_detected_flag = 0;

void IRSensor_Init(void) {
    P2DIR &= ~BIT0;
    P2REN |= BIT0;
    P2OUT |= BIT0;
    P2IES |= BIT0;
    P2IFG &= ~BIT0;
    P2IE &= ~BIT0;
}

void IRSensor_Arm(void) {
    pill_detected_flag = 0;
    P2IFG &= ~BIT0;
    P2IE |= BIT0;
}

void IRSensor_Disarm(void) {
    P2IE &= ~BIT0;
    P2IFG &= ~BIT0;
}

void __attribute__((interrupt(PORT2_VECTOR))) Port_2_ISR(void) {
    if (P2IFG & BIT0) {
        pill_detected_flag = 1;
        P2IFG &= ~BIT0;
    }
}