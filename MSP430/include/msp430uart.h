
#ifndef MSP430UART_H
#define MSP430UART_H

extern volatile unsigned char dispense1Flag;
extern volatile unsigned char dispense2Flag;
extern volatile unsigned char dispense1Count;
extern volatile unsigned char dispense2Count;

void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char *str);

#endif
