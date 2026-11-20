#ifndef STEPPER_H
#define STEPPER_H

#include <msp430.h>
#include <stdint.h>

void stepper_init(void);
void stepper_rotate(int steps);

#endif