#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#define MOTOR_MASK       0x0F
#define NUMBER_OF_SLOTS  6
#define STEPS_PER_SLOT   10  // Example only; calibrate your motor

void motor_init(void);
void motor_step(unsigned int steps);
void motor_select_slot(unsigned char target_slot);
unsigned char motor_get_current_slot(void);
void motor_release(void);

#endif