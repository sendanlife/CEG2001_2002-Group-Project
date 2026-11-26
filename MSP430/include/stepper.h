#ifndef STEPPER_H
#define STEPPER_H

#define STEPPER_SMALL 1
#define STEPPER_LARGE 0

void stepper_init(void);
<<<<<<< HEAD
void stepper_rotate(unsigned char motor, unsigned int steps);
=======
void stepper1Step(void);
void stepper2Step(void);
void stepper1Release(void);
void stepper2Release(void);
void stepper1Rotate(int steps);
void stepper2Rotate(int steps);
>>>>>>> 4ae0d6afe5619732ed3bf239f7bd9b3ef93233b9

#endif