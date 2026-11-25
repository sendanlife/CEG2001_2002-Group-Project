#ifndef STEPPER_H
#define STEPPER_H

#define STEPPER_SMALL 1
#define STEPPER_LARGE 0

void stepper_init(void);
void stepper_rotate(unsigned char motor, unsigned int steps);

#endif