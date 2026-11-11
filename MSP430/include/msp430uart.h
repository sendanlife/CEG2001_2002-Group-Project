#ifndef MSP430UART_H
#define MSP430UART_H

#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Summary: Header file for MSP430 UART communication. Declares 
 * initialization, string transmission and message reception functions used to
 * interface with the ESP32.
 * Pins: P3.3 = UCA0TXD -> ESP32 IO16 (RX2),  P3.4 = UCA0RXD <- ESP32 IO17 (TX2)
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

/*
 * Role: Configures USCI_A0 for UART communication (9600 baud, 8N1).
 * Parameters: None
 * Returns: None
 */
void msp430_uart_init(void);

/*
 * Role: Transmits a null-terminated string over UART to the ESP32.
 * Parameters: const char* str - pointer to the string to transmit
 * Returns: None
 */
void msp430_uart_send_string(const char* str);

/*
 * Role: Tells whether a complete line (ended by newline) has been received.
 * Parameters: None
 * Returns: true if a message is waiting to be read
 */
bool msp430_uart_message_ready(void);

/*
 * Role: Copies the received line (without the newline) into out and clears the flag.
 * Parameters: out - destination buffer, len - size of the buffer
 * Returns: true if a message was available
 */
bool msp430_uart_read_message(char* out, uint8_t len);

#endif /* MSP430UART_H */
