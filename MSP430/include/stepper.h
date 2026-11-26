
#ifndef STEPPER_H
#define STEPPER_H

#include <msp430.h>
#include <stdint.h>

void stepper_init(void);
void stepper1Step(void);
void stepper2Step(void);
void stepper1Release(void);
void stepper2Release(void);
void stepper1Rotate(int steps);
void stepper2Rotate(int steps);

#endif
