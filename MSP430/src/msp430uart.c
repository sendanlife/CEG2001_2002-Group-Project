#include "msp430uart.h"
#include <msp430.h>

volatile unsigned char dispense1Flag = 0;
volatile unsigned char dispense2Flag = 0;
volatile unsigned char dispense1Count = 0;
volatile unsigned char dispense2Count = 0;

void UART_Init(void) {
    P1DIR |= BIT0; 
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
        case 2: {
            char rx = UCA0RXBUF;
            P1OUT ^= BIT0;         // toggles on ANY received byte
            if (rx == '1') {
                dispense1Flag = 1;
            } else if (rx == '2') {
                dispense2Flag = 1;
            } else if (rx == 'D') {
                dispense1Flag = 1;
                dispense2Flag = 1;
            }
            break;
        }
        default:
            break;
    }
}