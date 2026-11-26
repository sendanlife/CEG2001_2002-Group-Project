#ifndef IRSENSOR_H
#define IRSENSOR_H

extern volatile unsigned char pill_detected_flag1; 
extern volatile unsigned char pill_detected_flag2;  

void IRSensor_Init(void);
void IRSensor_Arm(unsigned char motor);     
void IRSensor_Disarm(unsigned char motor);    
unsigned char IRSensor_PillDetected(unsigned char motor);

#endif