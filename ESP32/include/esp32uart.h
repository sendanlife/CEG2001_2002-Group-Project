#ifndef ESP32UART_H
#define ESP32UART_H

#include <Arduino.h>

/*
 * Summary: Header file for ESP32 UART communication with the MSP430 Launchpad.
 * Declares initialization functions and command handlers using Hardware Serial2.
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

/*
 * Role: Initializes Serial2 for communication with the MSP430 at 9600 baud.
 * Parameters: None
 * Returns: None
 */
void esp32_uart_init(void);

/*
 * Role: Transmits an authorization command over Serial2 to wake the MSP430.
 * Parameters: None
 * Returns: None
 */
void esp32_send_authorization(void);

/*
 * Role: Checks for and processes incoming messages from the MSP430 over UART.
 * Parameters: None
 * Returns: String - the trimmed message received from the MSP430
 */
String esp32_check_messages(void);

#endif /* ESP32UART_H */