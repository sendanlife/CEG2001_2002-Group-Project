#include <msp430.h>
#include <string.h>
#include "msp430uart.h"
#include "linearActuator.h"
#include "stepperMotor.h"

/*
 * Summary: Main application program for the SmartDispense MSP430 subsystem.
 * Initializes system peripherals, enters Low-Power Mode (LPM0) to conserve energy, 
 * and waits for a UART interrupt triggered by the ESP32 upon successful NFC 
 * patient authentication. Once awakened, it executes the dispensing sequence 
 * and transmits a confirmation string back to the ESP32.
 * Author(s): OWEN LIM, RACHELLE ANG RUI XUAN, GABRIEL LEE JUN RONG, LIEW HUI ZHONG, SYAFIQ BIN ISKANDAR ABDULLAH YEOH
 */

#define NUM_SLOTS 6

// Global variables for UART communication
volatile unsigned char dispense_flag = 0;
char rx_buffer[20];
int rx_index = 0;

// We use the UART init from msp430uart.h, so we don't need to redefine it here.
// Just make sure msp430uart.c configures the baud rate to 9600!

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

    // 1. Initialize all hardware modules
    setup_pins_and_timer();         // Initialize linear actuator pins and PWM
    motor_init();                   // Initialize stepper motor pins
    msp430_uart_init();             // Initialize UART communication interface
    
    __enable_interrupt();           // Enable global maskable interrupts

    while(1) {
        // Sleep and wait for UART interrupt
        __bis_SR_register(LPM0_bits + GIE); 
        
        // --- Execution resumes here after ESP32 sends "DISPENSE" ---
        if (dispense_flag) {
            
            // ACTION 1: Linear Actuator Sequence
            extend_actuator();            
            __delay_cycles(2000000);      // Wait ~2s for extension
            stop_actuator();              
            __delay_cycles(500000);       
            retract_actuator();           
            __delay_cycles(2000000);      // Wait ~2s for retraction
            stop_actuator();              

            // ACTION 2: Stepper Motor Sequence (Rotate to next slot)
            unsigned char target_slot = (motor_get_current_slot() + 1) % NUM_SLOTS;
            motor_select_slot(target_slot);
            motor_release();              // Turn off coils to prevent overheating

            // ACTION 3: Notify ESP32
            msp430_uart_send_string("DROP_SUCCESS\n"); 
            
            // Reset flag for next time
            dispense_flag = 0; 
        }
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
            dispense_flag = 1; // Set flag to trigger actions in main loop
            __bic_SR_register_on_exit(LPM0_bits); // Wake up CPU
        } else if (strcmp(rx_buffer, "AUTH") == 0) {
            __bic_SR_register_on_exit(LPM0_bits); // Wake up CPU just for testing
        }
        
        rx_index = 0; // Reset buffer
    } else {
        if (rx_index < 20) {
            rx_buffer[rx_index++] = received_char;
        }
    }
}