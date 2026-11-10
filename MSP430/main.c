#include <msp430.h>
#include <string.h>
#include "stepperMotor.h"

#define NUM_SLOTS 6
#define MAX_SLOTS 6

volatile unsigned char target_slot = 0;
volatile unsigned char dispense_flag = 0;
char rx_buffer[20];
int rx_index = 0;

void UART_Init(void) {
    // Configure UART for 9600 baud (assuming 1MHz SMCLK)
    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    UCA0BR0 = 104;        // 1MHz / 9600 = ~104
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS0;
    
    // Configure UART Pins (Example: P1.1 RX, P1.2 TX - check your board!)
    // P1SEL |= BIT1 + BIT2;
    // P1SEL2 |= BIT1 + BIT2;
    
    UCA0CTL1 &= ~UCSWRST;
    UCA0IE |= UCRXIE; // Enable RX interrupt
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    // Initialize hardware
    motor_init();
    UART_Init();

    __enable_interrupt(); // Enable global interrupts

    while (1) {
        if (dispense_flag) {
            // Calculate next slot (cycles 0 -> 1 -> 2 ... -> 5 -> 0)
            target_slot = (motor_get_current_slot() + 1) % NUMBER_OF_SLOTS;
            
            // Move motor to target slot
            motor_select_slot(target_slot);
            
            // Turn off motor coils so it doesn't overheat
            motor_release(); 
            
            dispense_flag = 0; // Reset flag
        }
        
        // Sleep until next interrupt
        __bis_SR_register(LPM0_bits + GIE); 
    }
}

// UART RX Interrupt
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    char received_char = UCA0RXBUF;
    
    if (received_char == '\n') {
        rx_buffer[rx_index] = '\0'; // Null terminate string
        
        // Check for ESP32 command
        if (strcmp(rx_buffer, "DISPENSE") == 0) {
            dispense_flag = 1; // Set flag to trigger motor in main loop
            __bic_SR_register_on_exit(LPM0_bits); // Wake up CPU
        }
        rx_index = 0; // Reset buffer
    } else {
        if (rx_index < 20) {
            rx_buffer[rx_index++] = received_char;
        }
    }
}