#include "msp430uart.h"

/*
 * Summary: Implements UART communication functions for the MSP430 using the 
 * USCI module. Configures baud rate for 9600 bps, sets up TX/RX pins, and 
 * implements the USCI_A0 receive interrupt service routine (ISR) to catch 
 * messages (e.g. "AUTH_OK") from the ESP32 and wake the MCU from low-power sleep.
 * Code Structure: msp430_uart_init(), msp430_uart_send_string(),
 *                 msp430_uart_message_ready(), msp430_uart_read_message(), USCI0RX_ISR
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

#define RX_BUF_LEN 32

static volatile char    rx_buf[RX_BUF_LEN];   /* line being received            */
static volatile uint8_t rx_idx = 0;
static volatile bool    msg_ready = false;    /* a complete line is waiting     */

/*
 * Role: Configures USCI_A0 for UART communication at 9600 baud.
 * Parameters: None
 * Returns: None
 */
void msp430_uart_init(void) {
    UCA0CTL1 |= UCSWRST;              /* Put USCI state machine in reset */
    UCA0CTL1 |= UCSSEL_2;             /* Use SMCLK as clock source */
    
    /* Baud rate configuration for the LaunchPad's default SMCLK (~1.048576 MHz), 9600 baud.
       (BR = 109, BRS = 2 gives ~0.2 % error; the old 1 MHz values 104/1 would be ~5 % off.) */
    UCA0BR0 = 109;                    /* Set lower baud rate register for 9600 */
    UCA0BR1 = 0;                      /* Clear upper baud rate register */
    UCA0MCTL = UCBRS_2 + UCBRF_0;     /* Set modulation stage settings */
    
    P3SEL |= BIT3 | BIT4;             /* Assign P3.3 (TX) and P3.4 (RX) to USCI_A0 */
    UCA0CTL1 &= ~UCSWRST;             /* Initialize USCI state machine */
    UCA0IE |= UCRXIE;                 /* Enable USCI_A0 receive interrupt */
}

/*
 * Role: Transmits a null-terminated string over UART to the ESP32.
 * Parameters: const char* str - pointer to the string to transmit
 * Returns: None
 */
void msp430_uart_send_string(const char* str) {
    while (*str) {
        while (!(UCA0IFG & UCTXIFG)); /* Wait until transmission buffer is ready */
        UCA0TXBUF = *str++;           /* Send current character and advance pointer */
    }
}

bool msp430_uart_message_ready(void) {
    return msg_ready;
}

bool msp430_uart_read_message(char* out, uint8_t len) {
    uint8_t i;

    if (!msg_ready || len == 0)
        return false;

    for (i = 0; i < len - 1 && rx_buf[i] != '\0'; i++)
        out[i] = rx_buf[i];
    out[i] = '\0';

    rx_idx = 0;
    msg_ready = false;                /* ready to receive the next line */
    return true;
}

/*
 * Role: ISR for USCI_A0 receive, collects incoming characters from the ESP32 into a line.
 * Parameters: None
 * Returns: None
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI0RX_ISR(void) {
    switch (__even_in_range(UCA0IV, 4)) {
        case 2: {                                   /* RX interrupt */
            char rx_char = UCA0RXBUF;               /* Read incoming byte (clears the flag) */

            if (msg_ready)                          /* previous line not read yet: ignore */
                break;

            if (rx_char == '\n') {                  /* end of line */
                if (rx_idx > 0) {
                    rx_buf[rx_idx] = '\0';
                    msg_ready = true;
                    __bic_SR_register_on_exit(LPM0_bits); /* wake main loop from LPM0 */
                }
            } else if (rx_char != '\r') {           /* ignore carriage return */
                if (rx_idx < RX_BUF_LEN - 1)
                    rx_buf[rx_idx++] = rx_char;
            }
            break;
        }
        default:
            break;
    }
}
