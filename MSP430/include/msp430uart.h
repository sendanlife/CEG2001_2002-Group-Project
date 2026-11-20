#ifndef MSP430UART_H
#define MSP430UART_H

extern volatile unsigned char dispense_flag;

void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char *str);

#endif