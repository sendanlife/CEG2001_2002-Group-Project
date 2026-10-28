#include <msp430.h>
#include <stdbool.h>
#include "irSensor.h"

/* ====================================================================
 * PIN DEFINITIONS (Red LaunchPad Board)
 * ==================================================================== */
// 1. IR Sensor: OUT pin goes to P2.0
#define IR_PIN              BIT0
#define IR_PORT_DIR         P2DIR
#define IR_PORT_REN         P2REN
#define IR_PORT_OUT         P2OUT
#define IR_PORT_IN          P2IN

// 2. Built-in Green Light on the Red Board (LED2 is on P4.7)
#define LED_GREEN_PIN       BIT7
#define LED_PORT_DIR        P4DIR
#define LED_PORT_OUT        P4OUT

/* ====================================================================
 * UART TALK FUNCTION (P4.4 TXD -> ESP32)
 * ==================================================================== */
void UART_sendSuccess(void) {
    const char *msg = "Success\r\n";
    while (*msg) {
        while (!(UCA1IFG & UCTXIFG)); // Wait until the wire is clear
        UCA1TXBUF = *msg++;           // Send each letter
    }
}

/* ====================================================================
 * SETUP: WAKE UP THE PINS
 * ==================================================================== */
void init_IRSensor(void) {
    // 1. Tell P2.0 to be a listener (Input) for the IR sensor
    IR_PORT_DIR &= ~IR_PIN;
    IR_PORT_REN |= IR_PIN;            // Turn on helper pull-up spring
    IR_PORT_OUT |= IR_PIN;

    // 2. Tell P4.7 to control the Red Board's built-in GREEN LED
    LED_PORT_DIR |= LED_GREEN_PIN;
    LED_PORT_OUT &= ~LED_GREEN_PIN;   // Start with the green light OFF

    // 3. Set up the talk line pins (P4.4 = TX, P4.5 = RX)
    P4SEL |= BIT4 | BIT5;
    UCA1CTL1 |= UCSWRST;              // Pause to configure
    UCA1CTL1 |= UCSSEL_2;             // SMCLK clock source
    UCA1BR0 = 9;                      // 115200 speed
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;
    UCA1CTL1 &= ~UCSWRST;             // Ready to talk!
}

/* ====================================================================
 * FLOWCHART: Pill Dropped? -> Yes / No
 * ==================================================================== */
void check_Pill_Dropped(void) {
    // Active LOW: Sensor line drops to 0 when it sees a pill
    bool pill_dropped = ((IR_PORT_IN & IR_PIN) == 0);

    // Flowchart: "IR Sensor: Pill Dropped?"
    if (pill_dropped) {
        /* ----- YES BRANCH ----- */
        // 1. Turn the Red Board's built-in LED green!
        LED_PORT_OUT |= LED_GREEN_PIN;

        // 2. Send "Success" through the wire to the ESP32!
        UART_sendSuccess();
    }
    else {
        /* ----- NO BRANCH ----- */
        // Leave empty! Do nothing at all.
    }
}