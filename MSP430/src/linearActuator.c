#include <msp430.h>
#include "linearActuator.h"

/*
 * Summary: This modular embedded C program drives a 12V linear actuator
 * using an L298N motor driver connected to an MSP430 Launchpad. The program
 * configures P1.2 for Timer_A PWM output to control the L298N's ENA pin
 * (speed control), while P1.3 and P1.4 act as digital outputs to control IN1 
 * and IN2 (direction control). 
 * Code Structure: setup_pins_and_timer(), extend_actuator(), retract_actuator(), stop_actuator()
 * Author(s): Liew Hui Zhong
 */

/*
 * Role: Initializes the GPIO pins and configures Timer_A for PWM generation.
 * Parameters: None
 * Returns: None
 */
void setup_pins_and_timer(void) {
    // Set P1.2 as output for PWM (ENA), P1.3 for IN1, P1.4 for IN2
    P1DIR |= BIT2 | BIT3 | BIT4;      /* Configure P1.2, P1.3, P1.4 as outputs */
    P1SEL |= BIT2;                    /* Select TA0.1 option for P1.2 PWM */

    // Configure Timer_A for PWM output
    TA0CCR0 = 1000 - 1;               /* Set PWM Period for 1kHz */
    TA0CCTL1 = OUTMOD_7;              /* Set CCR1 to reset/set mode for PWM */
    TA0CCR1 = 0;                      /* Initialize with 0% duty cycle */
    TA0CTL = TASSEL_2 + MC_1 + TACLR; /* Use SMCLK, UP mode, clear TAR */
}

/*
 * Role: Extends the linear actuator by setting IN1 High, IN2 Low, and applying PWM.
 * Parameters: None
 * Returns: None
 */
void extend_actuator(void) {
    P1OUT |= BIT3;                    /* Set IN1 High to push rod out */
    P1OUT &= ~BIT4;                   /* Set IN2 Low */
    TA0CCR1 = 900;                    /* Set PWM duty cycle to 90% speed */
}

/*
 * Role: Retracts the linear actuator by setting IN1 Low, IN2 High, and applying PWM.
 * Parameters: None
 * Returns: None
 */
void retract_actuator(void) {
    P1OUT &= ~BIT3;                   /* Set IN1 Low */
    P1OUT |= BIT4;                    /* Set IN2 High to pull rod back */
    TA0CCR1 = 900;                    /* Set PWM duty cycle to 90% speed */
}

/*
 * Role: Stops the linear actuator by turning off the PWM signal and clearing direction pins.
 * Parameters: None
 * Returns: None
 */
void stop_actuator(void) {
    TA0CCR1 = 0;                      /* Set PWM to 0% to cut motor power */
    P1OUT &= ~(BIT3 | BIT4);          /* Clear IN1 and IN2 digital pins */
}

/*
 * Individual Reflection:
 * Liew Hui Zhong: Designing the GPIO initialization was straightforward, but mapping out which pins supported 
 * Timer_A outputs based on the datasheet required direct application of CEG2002 hardware concepts.
 * Implementing the PWM required applying knowledge from lectures on Timer_A registers (CCR0 and CCR1). 
 * Debugging the duty cycle was a challenge until I correctly configured the OUTMOD_7 reset/set behavior.
 * Integrating the physical sequence into a cohesive C flow was tricky. I learned that adding delays 
 * between reversing the motor polarity is crucial to prevent voltage spikes that could crash the L298N.
 */