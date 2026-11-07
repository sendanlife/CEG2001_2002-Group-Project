#ifndef LINEAR_ACTUATOR_H
#define LINEAR_ACTUATOR_H

#include <msp430.h>

/*
 * Summary: Header file for linearActuator.c, providing function prototypes 
 * for controlling a 12V linear actuator via an L298N driver and MSP430 Timer_A.
 * Author(s): [Insert Team Member 1], [Insert Team Member 2], [Insert Team Member 3]
 * Version: 1.0
 */

/*
 * Role: Initializes GPIO pins and configures Timer_A for PWM generation.
 * Parameters: None
 * Returns: None
 */
void setup_pins_and_timer(void);

/*
 * Role: Extends the linear actuator push rod.
 * Parameters: None
 * Returns: None
 */
void extend_actuator(void);

/*
 * Role: Retracts the linear actuator push rod.
 * Parameters: None
 * Returns: None
 */
void retract_actuator(void);

/*
 * Role: Stops the linear actuator by cutting motor power and clearing pins.
 * Parameters: None
 * Returns: None
 */
void stop_actuator(void);

#endif /* LINEAR_ACTUATOR_H */
