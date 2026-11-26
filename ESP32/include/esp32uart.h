#ifndef ESP32UART_H
#define ESP32UART_H

#include <Arduino.h>

/*
 * Summary: Header file for ESP32 UART communication with the MSP430 Launchpad.
 * Declares initialization functions and command handlers using Hardware Serial2.
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

void esp32_uart_init(void);
void esp32_send_authorization(void);
String esp32_check_messages(void);

#endif 