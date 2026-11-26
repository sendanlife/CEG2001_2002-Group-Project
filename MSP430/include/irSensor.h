#ifndef IRSENSOR_H
#define IRSENSOR_H

extern volatile unsigned char pill_detected_flag1;   // sensor on P2.0 (stepper 1)
extern volatile unsigned char pill_detected_flag2;   // sensor on P2.2 (stepper 2)

void IRSensor_Init(void);
void IRSensor_Arm(unsigned char motor);       // motor = 1 or 2
void IRSensor_Disarm(unsigned char motor);    // motor = 1 or 2
unsigned char IRSensor_PillDetected(unsigned char motor);

#endif