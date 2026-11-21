#ifndef LOADCELL_H
#define LOADCELL_H

#define HX711_DOUT_DIR  P4DIR
#define HX711_DOUT_IN   P4IN
#define HX711_DOUT_BIT  BIT3

#define HX711_SCK_DIR   P4DIR
#define HX711_SCK_OUT   P4OUT
#define HX711_SCK_BIT   BIT0

#define HX711_SCALE_FACTOR 50.0f

void LoadCell_Init(void);
void LoadCell_Tare(unsigned char samples);
unsigned char LoadCell_IsReady(void);
long LoadCell_ReadRaw(void);
float LoadCell_RawToGrams(long rawAvg);

#endif