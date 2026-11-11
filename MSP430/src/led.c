#include <msp430.h>
#include "led.h"

/* Red LED pin: P2.2 (change these three lines to move it to another pin) */
#define LED_RED_DIR   P2DIR
#define LED_RED_OUT   P2OUT
#define LED_RED_PIN   BIT2

void LED_Init(void) {
    LED_RED_DIR |= LED_RED_PIN;     // Pin as output
    LED_RED_OUT &= ~LED_RED_PIN;    // Start with LED off
}

void LED_TurnOn(void) {
    LED_RED_OUT |= LED_RED_PIN;
}

void LED_TurnOff(void) {
    LED_RED_OUT &= ~LED_RED_PIN;
}
