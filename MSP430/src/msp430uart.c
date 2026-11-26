#include "msp430uart.h"
#include <msp430.h>

volatile unsigned char dispense1Flag = 0;
volatile unsigned char dispense2Flag = 0;
volatile unsigned char dispense1Count = 0;
volatile unsigned char dispense2Count = 0;

// pills to drop per motor for the current request (set by the ESP32)
volatile unsigned char expected1 = 1;
volatile unsigned char expected2 = 1;

static volatile unsigned char rxState = 0;   // 0 idle, 1 expect dose1, 2 expect dose2

void UART_Init(void) {
    P1DIR |= BIT0;              // red LED: toggles on every received byte
    P1OUT &= ~BIT0;

    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 |= UCSSEL_2;       // SMCLK
    UCA0BR0 = 109;              // 9600 baud @ ~1.05 MHz
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_2 | UCBRF_0;
    P3SEL |= BIT3 | BIT4;       // P3.3 = TXD, P3.4 = RXD
    UCA0CTL1 &= ~UCSWRST;
    UCA0IE |= UCRXIE;
}

void UART_SendChar(char c) {
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = c;
}

void UART_SendString(const char *str) {
    while (*str != '\0') {
        UART_SendChar(*str++);
    }
}

void __attribute__((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR(void) {
    switch (UCA0IV) {
        case 2: {
            char rx = UCA0RXBUF;
            P1OUT ^= BIT0;

            if (rxState == 1) {                       // first digit: dose for motor 1
                expected1 = (rx >= '0' && rx <= '9') ? (rx - '0') : 0;
                rxState = 2;
            } else if (rxState == 2) {                // second digit: dose for motor 2
                expected2 = (rx >= '0' && rx <= '9') ? (rx - '0') : 0;
                rxState = 0;
                if (expected1 > 0) dispense1Flag = 1;
                if (expected2 > 0) dispense2Flag = 1;
            } else if (rx == 'S') {
                rxState = 1;
            } else if (rx == '1') {                   // old commands, one pill each
                expected1 = 1; dispense1Flag = 1;
            } else if (rx == '2') {
                expected2 = 1; dispense2Flag = 1;
            } else if (rx == 'D') {
                expected1 = 1; expected2 = 1;
                dispense1Flag = 1; dispense2Flag = 1;
            }
            __bic_SR_register_on_exit(LPM3_bits);   // wake main() if it is sleeping (does nothing otherwise)
            break;
        }
        default:
            break;
    }
}