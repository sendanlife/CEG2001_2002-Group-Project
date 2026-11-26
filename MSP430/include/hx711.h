
#ifndef HX711_H
#define HX711_H

#include <msp430.h>
#include <stdint.h>

void HX711_Init(void);

uint8_t HX711_ReadRaw(long *result);

uint8_t HX711_ReadAverage(long *average, uint8_t samples);

#endif
