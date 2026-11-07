#include <msp430.h>
#include "msp430uart.h"
#include "linearActuator.h"

/*
 * Summary: Main application program for the SmartDispense MSP430 subsystem.
 * Initializes system peripherals, enters Low-Power Mode (LPM0) to conserve energy, 
 * and waits for a UART interrupt triggered by the ESP32 upon successful NFC 
 * patient authentication. Once awakened, it executes the linear actuator dispensing 
 * sequence and transmits a confirmation string back to the ESP32.
 * Code Structure: main()
 * Author(s): OWEN LIM, RACHELLE ANG RUI XUAN, GABRIEL LEE JUN RONG, LIEW HUI ZHONG, SYAFIQ BIN ISKANDAR ABDULLAH YEOH
 */

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;         /* Stop watchdog timer to prevent resets (Author: TM1) */
    
    setup_pins_and_timer();           /* Initialize actuator pins and Timer_A PWM (Author: TM2) */
    msp430_uart_init();               /* Initialize UART communication interface (Author: TM1) */
    
    __enable_interrupts();            /* Enable global maskable interrupts (Author: TM1) */

    while(1) {
        __bis_SR_register(LPM0_bits + GIE); /* Enter LPM0 with interrupts enabled */
        
        extend_actuator();            /* Push out the linear actuator to dispense pill (Author: TM3) */
        __delay_cycles(2000000);      /* Wait ~2s for complete extension stroke (Author: TM3) */
        
        stop_actuator();              /* Stop motor briefly to protect components (Author: TM3) */
        __delay_cycles(500000);       /* Brief pause before retraction (Author: TM3) */
        
        retract_actuator();           /* Pull back linear actuator to home position (Author: TM3) */
        __delay_cycles(2000000);      /* Wait ~2s for complete retraction (Author: TM3) */
        
        stop_actuator();              /* Cut power to actuator (Author: TM3) */
        
        msp430_uart_send_string("DROP_SUCCESS\n"); /* Transmit drop confirmation back to ESP32 (Author: TM1) */
    }
    
    return 0;
}