#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>
#include "irSensor.h"

/* ====================================================================
 * PIN DEFINITIONS (Red LaunchPad Board)
 * ==================================================================== */
// 1. IR Sensor: OUT pin goes to P1.3
//    (P2.0 is used by the AM2302 temperature sensor, so the IR sensor moved to P1.3)
#define IR_PIN              BIT3
#define IR_PORT_DIR         P1DIR
#define IR_PORT_REN         P1REN
#define IR_PORT_OUT         P1OUT
#define IR_PORT_IN          P1IN

// 2. Built-in Green Light on the Red Board (LED2 is on P4.7)
#define LED_GREEN_PIN       BIT7
#define LED_PORT_DIR        P4DIR
#define LED_PORT_OUT        P4OUT

// 3. Detection window: how long to watch for the pill after the dispenser moves
#define IR_DETECT_WINDOW_MS 3000
#define IR_CPU_HZ           1000000UL      /* default DCO ~1 MHz */

/* ====================================================================
 * UART TALK FUNCTION (P4.4 TXD -> ESP32)
 * ==================================================================== */
static void UART_sendSuccess(void) {
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
    // 1. P1.3 = input for the IR sensor, with pull-up
    IR_PORT_DIR &= ~IR_PIN;
    IR_PORT_REN |= IR_PIN;            // Turn on helper pull-up spring
    IR_PORT_OUT |= IR_PIN;

    // 2. P4.7 controls the Red Board's built-in GREEN LED
    LED_PORT_DIR |= LED_GREEN_PIN;
    LED_PORT_OUT &= ~LED_GREEN_PIN;   // Start with the green light OFF

    // 3. Set up the talk line pins (P4.4 = TX, P4.5 = RX), 115200 baud @ ~1.048 MHz SMCLK
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
bool check_Pill_Dropped(void) {
    uint16_t ms;

    // A falling pill passes the sensor very quickly, so watch the pin for a
    // window of time instead of checking it only once.
    for (ms = 0; ms < IR_DETECT_WINDOW_MS; ms++) {
        // Active LOW: Sensor line drops to 0 when it sees a pill
        if ((IR_PORT_IN & IR_PIN) == 0) {
            /* ----- YES BRANCH ----- */
            LED_PORT_OUT |= LED_GREEN_PIN;   // 1. Built-in LED green
            UART_sendSuccess();              // 2. Send "Success" to the ESP32
            return true;
        }
        __delay_cycles(IR_CPU_HZ / 1000UL);  // 1 ms
    }

    /* ----- NO BRANCH ----- */
    return false;                            // Caller continues with the jam/retry branch
}

void IRSensor_ClearLED(void) {
    LED_PORT_OUT &= ~LED_GREEN_PIN;
}
