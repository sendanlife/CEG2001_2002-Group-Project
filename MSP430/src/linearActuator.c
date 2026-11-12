#include <msp430.h>
#include "linearActuator.h"

#define ACTUATOR_CPU_HZ   1000000UL     /* default DCO ~1 MHz */

static void delay_ms(unsigned int ms) {
    while (ms--) {
        __delay_cycles(ACTUATOR_CPU_HZ / 1000UL);
    }
}

/*
 * Summary: This modular embedded C program drives a 12V linear actuator
 * using an L298N motor driver connected to an MSP430 Launchpad. The program
 * configures P1.2 for Timer_A PWM output to control the L298N's ENA pin
 * (speed control), while P1.5 and P1.4 act as digital outputs to control IN1 
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
    // Set P1.2 as output for PWM (ENA), P1.5 for IN1, P1.4 for IN2
    P1DIR |= BIT2 | BIT5 | BIT4;      /* Configure P1.2, P1.5, P1.4 as outputs */
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
    P1OUT |= BIT5;                    /* Set IN1 High to push rod out */
    P1OUT &= ~BIT4;                   /* Set IN2 Low */
    TA0CCR1 = 900;                    /* Set PWM duty cycle to 90% speed */
}

/*
 * Role: Retracts the linear actuator by setting IN1 Low, IN2 High, and applying PWM.
 * Parameters: None
 * Returns: None
 */
void retract_actuator(void) {
    P1OUT &= ~BIT5;                   /* Set IN1 Low */
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
    P1OUT &= ~(BIT5 | BIT4);          /* Clear IN1 and IN2 digital pins */
}

/*
 * Role: Extends the actuator for a fixed time, then stops it.
 * Parameters: ms - run time in milliseconds
 * Returns: None
 */
void extend_actuator_for(unsigned int ms) {
    extend_actuator();
    delay_ms(ms);
    stop_actuator();
}

/*
 * Role: Retracts the actuator for a fixed time, then stops it.
 * Parameters: ms - run time in milliseconds
 * Returns: None
 */
void retract_actuator_for(unsigned int ms) {
    retract_actuator();
    delay_ms(ms);
    stop_actuator();
}

/*
 * Role: Extends the actuator to ACTUATOR_MAX_EXTEND_PERCENT of its stroke (time based), then stops.
 * Parameters: None
 * Returns: None
 */
void extend_actuator_limited(void) {
    unsigned int ms = (unsigned int)(((unsigned long)ACTUATOR_FULL_STROKE_MS *
                                      ACTUATOR_MAX_EXTEND_PERCENT) / 100UL);
    extend_actuator_for(ms);
}

/*
 * Role: Retracts for the full stroke time so the rod ends fully retracted (re-homes every cycle).
 * Parameters: None
 * Returns: None
 */
void retract_actuator_home(void) {
    retract_actuator_for(ACTUATOR_FULL_STROKE_MS);
}