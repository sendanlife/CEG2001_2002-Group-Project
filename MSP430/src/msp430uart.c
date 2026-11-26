
#include "msp430uart.h"
#include <msp430.h>

volatile unsigned char dispense_flag = 0;

/* UART UCA1
   TX: P4.4
   RX: P4.5
   Baud: 9600
*/

void UART_Init(void)
{
    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 |= UCSSEL_2;

    UCA1BR0 = 6;
    UCA1BR1 = 0;
    UCA1MCTL = UCBRF_13 | UCOS16;

    P4SEL |= BIT4 | BIT5;

    UCA1CTL1 &= ~UCSWRST;

    UCA1IE |= UCRXIE;
}

void UART_SendChar(char c)
{
    while (!(UCA1IFG & UCTXIFG));

    UCA1TXBUF = c;
}

void UART_SendString(const char *str)
{
    while (*str != '\0')
    {
        UART_SendChar(*str++);
    }
}

/* UART receive interrupt */
void __attribute__((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR(void)
{
    switch (UCA1IV)
    {
        case 2:
            if (UCA1RXBUF == 'D')
            {
                dispense_flag = 1;
            }
            break;

        default:
            break;
    }
}
