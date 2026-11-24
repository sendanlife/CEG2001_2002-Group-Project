#ifndef IRSENSOR_H
#define IRSENSOR_H

extern volatile unsigned char pill_detected_flag;

void IRSensor_Init(void);
void IRSensor_Arm(void);
void IRSensor_Disarm(void);

#endif