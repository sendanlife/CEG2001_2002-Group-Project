#include "msp430uart.h"
#include <msp430.h>

volatile unsigned char dispense_flag = 0;

void UART_Init(void) {
    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 |= UCSSEL_2;
    UCA0BR0 = 109;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_2 | UCBRF_0;
    P3SEL |= BIT3 | BIT4;
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
        case 2:
            if (UCA0RXBUF == 'D') {
                dispense_flag = 1;
            }
            break;
        default:
            break;
    }
}
