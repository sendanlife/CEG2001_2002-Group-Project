#ifndef LOADCELL_H
#define LOADCELL_H

// ---- HX711 wiring (unchanged) ----
#define HX711_DOUT_DIR  P3DIR
#define HX711_DOUT_IN   P3IN
#define HX711_DOUT_OUT  P3OUT
#define HX711_DOUT_REN  P3REN
#define HX711_DOUT_SEL  P3SEL
#define HX711_DOUT_BIT  BIT6     // P3.6 <- HX711 DOUT

#define HX711_SCK_DIR   P3DIR
#define HX711_SCK_OUT   P3OUT
#define HX711_SCK_SEL   P3SEL
#define HX711_SCK_BIT   BIT5     // P3.5 -> HX711 SCK

// ---- calibration (same numbers as the Arduino test sketch) ----
// Raw counts per gram:  grams = (raw - tare offset) / HX711_SCALE_FACTOR
// This only holds for the same load cell, gain 128 and the same excitation voltage
// as when it was calibrated. If the HX711 now runs from 3.3 V instead of 5 V, re-check it
// with a known weight (see the notes in the answer).
#define HX711_SCALE_FACTOR   1096.50f

#define ZERO_DEADBAND_G      0.2f    // |weight| <= 0.2 g is shown as 0.0 g
#define FILTER_SIZE          5       // moving average over the last 5 averaged readings
#define FILTER_READ_SAMPLES  5       // raw readings averaged per filter sample (get_units(5))

void LoadCell_Init(void);
void LoadCell_Tare(unsigned char samples);          // zero the scale: it must be empty and still
unsigned char LoadCell_IsReady(void);               // 1 when a conversion is waiting
unsigned char LoadCell_IsConnected(void);           // 0 once the HX711 has stopped answering
long LoadCell_ReadRaw(void);                        // one raw 24-bit reading (signed)
long LoadCell_ReadAverage(unsigned char samples);   // average of N raw readings (N <= 100)
float LoadCell_RawToGrams(long rawAvg);             // raw -> grams, tare and deadband applied (can be negative)
float LoadCell_GetGrams(unsigned char samples);     // like scale.get_units(samples)
float LoadCell_GetSmoothedGrams(void);              // get_units(5) + 5-point moving average + deadband (blocks ~0.5 s)
float LoadCell_SmoothGrams(long rawAvg);            // same filter, but you give it the averaged raw value (no waiting)
void LoadCell_ResetFilter(void);

#endif