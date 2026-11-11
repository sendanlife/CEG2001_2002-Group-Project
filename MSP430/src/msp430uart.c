#include "msp430uart.h"

/*
 * Summary: Implements UART communication functions for the MSP430 using the 
 * USCI module. Configures baud rate for 9600 bps, sets up TX/RX pins, and 
 * implements the USCI_A0 receive interrupt service routine (ISR) to catch 
 * authorization flags from the ESP32 and wake the MCU from low-power sleep.
 * Code Structure: msp430_uart_init(), msp430_uart_send_string(), USCI0RX_ISR
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

/*
 * Role: Configures USCI_A0 for UART communication at 9600 baud.
 * Parameters: None
 * Returns: None
 */
void msp430_uart_init(void) {
    UCA0CTL1 |= UCSWRST;              /* Put USCI state machine in reset (Author: TM1) */
    UCA0CTL1 |= UCSSEL_2;             /* Use SMCLK as clock source (Author: TM1) */
    
    /* Baud rate configuration for 1MHz SMCLK, 9600 baud */
    UCA0BR0 = 104;                    /* Set lower baud rate register for 9600 (Author: TM1) */
    UCA0BR1 = 0;                      /* Clear upper baud rate register (Author: TM1) */
    UCA0MCTL = UCBRS_1 + UCBRF_0;     /* Set modulation stage settings (Author: TM1) */
    
    P3SEL |= BIT3 | BIT4;             /* Assign P3.3 (TX) and P3.4 (RX) to USCI_A0 (Author: TM1) */
    UCA0CTL1 &= ~UCSWRST;             /* Initialize USCI state machine (Author: TM1) */
    UCA0IE |= UCRXIE;               /* Enable USCI_A0 receive interrupt (Author: TM1) */
}

/*
 * Role: Transmits a null-terminated string over UART to the ESP32.
 * Parameters: char* str - pointer to the string to transmit
 * Returns: None
 */
void msp430_uart_send_string(char* str) {
    while (*str) {
        while (!(UCA0IFG & UCTXIFG)); /* Wait until transmission buffer is ready (Author: TM2) */
        UCA0TXBUF = *str++;           /* Send current character and advance pointer (Author: TM2) */
    }
}

/*
 * Role: ISR for USCI_A0 receive, handles incoming commands from ESP32.
 * Parameters: None
 * Returns: None
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI0RX_ISR(void) {
    char rx_char = UCA0RXBUF;         /* Read incoming byte from receive buffer (Author: TM1) */
    
    /* If newline or carriage return received, wake up main CPU from LPM */
    if (rx_char == '\n' || rx_char == '\r') {
        __bic_SR_register_on_exit(LPM0_bits); /* Clear LPM0 bits on exit to resume main loop (Author: TM1) */
    }
}
