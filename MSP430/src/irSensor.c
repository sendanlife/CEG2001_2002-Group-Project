#include <msp430.h>
#include "irSensor.h"

#define IR_ALL_MASK (IR_SMALL_BIT | IR_LARGE_BIT)

volatile unsigned char pill_detected_flag = 0;
static volatile unsigned char armed_bit = 0;

void IRSensor_Init(void) {
    P2SEL &= ~IR_ALL_MASK;   // GPIO function on P2.0 and P2.2
    P2DIR &= ~IR_ALL_MASK;   // Inputs

    // Internal pull-ups + falling-edge trigger (active-LOW IR output)
    P2REN |= IR_ALL_MASK;
    P2OUT |= IR_ALL_MASK;
    P2IES |= IR_ALL_MASK;

    P2IE  &= ~IR_ALL_MASK;   // Keep both disarmed until dispensing starts
    P2IFG &= ~IR_ALL_MASK;   // Clear stale flags after configuring P2IES
}

void IRSensor_Arm(unsigned char channel) {
    unsigned char bit = (channel == IR_CHANNEL_LARGE) ? IR_LARGE_BIT : IR_SMALL_BIT;

    P2IE  &= ~IR_ALL_MASK;   // Ensure only one sensor is armed at a time
    pill_detected_flag = 0;
    armed_bit = bit;

    P2IFG &= ~bit;           // Clear any stale edge on the target pin
    P2IE  |= bit;            // Enable interrupt on P2.0 or P2.2
}

void IRSensor_Disarm(void) {
    P2IE  &= ~IR_ALL_MASK;
    P2IFG &= ~IR_ALL_MASK;
    armed_bit = 0;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(PORT2_VECTOR))) Port_2_ISR(void)
#else
#error Compiler not supported!
#endif
{
    unsigned char bit = armed_bit;
    if (bit && (P2IFG & bit) && (P2IE & bit)) {
        pill_detected_flag = 1;
        P2IFG &= ~bit;
        P2IE  &= ~bit;       // Latch first drop to ignore bounce
    } else {
        // Clear any stray flag on IR pins so the ISR never re-enters infinitely
        P2IFG &= ~IR_ALL_MASK;
    }
}