#include <msp430.h>
#include "statusIndicator.h"

#define STATUS_LED_MASK (LED_RED_BIT | LED_YELLOW_BIT | LED_GREEN_BIT)
#define STATUS_ALL_MASK (STATUS_LED_MASK | BUZZER_BIT)

static void delayMs(unsigned int ms) {
    while (ms--) {
        __delay_cycles(1000);   
    }
}

static void buzzerTone(unsigned int durationMs) {
#if BUZZER_IS_ACTIVE
    P2OUT |= BUZZER_BIT;
    delayMs(durationMs);
    P2OUT &= ~BUZZER_BIT;
#else
    unsigned long periods = ((unsigned long)durationMs * 5UL) / 2UL;
    while (periods--) {
        P2OUT |= BUZZER_BIT;
        __delay_cycles(200);
        P2OUT &= ~BUZZER_BIT;
        __delay_cycles(200);
    }
#endif
}

void Status_Init(void) {
    P2SEL &= ~STATUS_ALL_MASK;
    P2IE  &= ~STATUS_ALL_MASK;
    P2DIR |= STATUS_ALL_MASK;
    P2DS  |= BUZZER_BIT;       
    P2OUT &= ~STATUS_ALL_MASK;
}

void Status_ClearAll(void) {
    P2OUT &= ~STATUS_LED_MASK;
}

void Status_SetDispensing(void) {
    P2OUT = (P2OUT & ~STATUS_LED_MASK) | LED_YELLOW_BIT;
}

void Status_SetDispensed(void) {
    P2OUT = (P2OUT & ~STATUS_LED_MASK) | LED_GREEN_BIT | BUZZER_BIT;
}

void Status_SetFail(void) {
    P2OUT = (P2OUT & ~STATUS_LED_MASK) | LED_RED_BIT;
}

void Status_BeepComplete(void) {
    buzzerTone(150);
    delayMs(100);
    buzzerTone(150);
}