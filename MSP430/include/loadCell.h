#ifndef LOADCELL_H
#define LOADCELL_H

#define HX711_DOUT_DIR  P3DIR
#define HX711_DOUT_IN   P3IN
#define HX711_DOUT_OUT  P3OUT
#define HX711_DOUT_REN  P3REN
#define HX711_DOUT_SEL  P3SEL
#define HX711_DOUT_BIT  BIT6

#define HX711_SCK_DIR   P3DIR
#define HX711_SCK_OUT   P3OUT
#define HX711_SCK_SEL   P3SEL
#define HX711_SCK_BIT   BIT5

// Update this with your calibrated counts-per-gram value
#define HX711_SCALE_FACTOR 89.0f

void LoadCell_Init(void);
void LoadCell_Tare(unsigned char samples);
unsigned char LoadCell_IsReady(void);
long LoadCell_ReadRaw(void);
float LoadCell_RawToGrams(long rawAvg);

#endif