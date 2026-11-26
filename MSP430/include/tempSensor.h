#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

extern volatile unsigned char temp_ready_flag;
extern volatile unsigned int rawADC;

void TempSensor_Init(void);
float TempSensor_GetLatestReading(void);

#endif