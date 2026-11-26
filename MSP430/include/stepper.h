
#ifndef STEPPER_H
#define STEPPER_H

#include <msp430.h>
#include <stdint.h>

void stepper_init(void);
void stepper_rotate(int steps);
void stepper2_rotate(int steps);
void steppers_rotate_together(int steps);

#endif
