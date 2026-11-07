#ifndef MSP430UART_H
#define MSP430UART_H

#include <msp430.h>

/*
 * Summary: Header file for MSP430 UART communication. Declares 
 * initialization and string transmission functions used to interface with the ESP32.
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 * Version: 1.1
 */

/*
 * Role: Configures USCI_A0 for UART communication.
 * Parameters: None
 * Returns: None
 */
void msp430_uart_init(void);

/*
 * Role: Transmits a null-terminated string over UART to the ESP32.
 * Parameters: char* str - pointer to the string to transmit
 * Returns: None
 */
void msp430_uart_send_string(char* str);

#endif /* MSP430UART_H */