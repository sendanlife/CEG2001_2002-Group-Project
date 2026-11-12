#include <msp430.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    // Configure P1.0 (Red LED on MSP430F5529 LaunchPad) as output
    P1DIR |= BIT0;              
    P1OUT &= ~BIT0;             // Start with LED off

    while(1) {
        P1OUT ^= BIT0;          // Toggle the LED state
        __delay_cycles(500000); // Delay for ~0.5 seconds
    }
}