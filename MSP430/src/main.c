#ifndef LINEAR_ACTUATOR_H
#define LINEAR_ACTUATOR_H

#include <msp430.h>

/*
 * Summary: Header file for linearActuator.c, providing function prototypes 
 * for controlling a 12V linear actuator via an L298N driver and MSP430 Timer_A.
 * Pins: ENA = P1.2 (TA0.1 PWM), IN1 = P1.5, IN2 = P1.4
 * Author(s): [Insert Team Member 1], [Insert Team Member 2], [Insert Team Member 3]
 */

/*
 * Time (ms) the actuator needs to travel its full stroke at the PWM speed set in
 * linearActuator.c. MEASURE THIS on your actuator - 3000 is only a placeholder.
 */
#define ACTUATOR_FULL_STROKE_MS   3000

/*
 * The rod is only allowed to extend this percentage of the full stroke (never the max length).
 */
#define ACTUATOR_MAX_EXTEND_PERCENT   90

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

/*
 * Role: Extends the push rod for a fixed time, then stops.
 * Parameters: ms - run time in milliseconds
 * Returns: None
 */
void extend_actuator_for(unsigned int ms);

/*
 * Role: Retracts the push rod for a fixed time, then stops.
 * Parameters: ms - run time in milliseconds
 * Returns: None
 */
void retract_actuator_for(unsigned int ms);

/*
 * Role: Extends the push rod to ACTUATOR_MAX_EXTEND_PERCENT (90%) of the full stroke, then stops.
 *       Assumes the rod starts fully retracted (call retract_actuator_home() first if unsure).
 * Parameters: None
 * Returns: None
 */
void extend_actuator_limited(void);

/*
 * Role: Retracts the push rod for the full stroke time so it ends fully retracted (home position).
 * Parameters: None
 * Returns: None
 */
void retract_actuator_home(void);

#endif
