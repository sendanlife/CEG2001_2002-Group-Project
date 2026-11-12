#ifndef LOADCELL_H
#define LOADCELL_H

/* Load cell via XFW-HX711 module on MSP430F5529 (uses hx711.c / hx711.h)
   Pins are fixed at compile time in hx711.h:
     HX711 DOUT -> P4.0,  HX711 SCK -> P4.1,  VCC -> 3.3V,  GND -> GND            */

/* Calibration factor = counts per gram. The ESP32/Arduino value (2280.0) is only a
   placeholder - replace it with your own (see main_load_test.c for how to find it). */
#define LOADCELL_SCALE      2280.0f
#define LOADCELL_TARE_SAMPLES    20     /* averaged readings when taring   */
#define LOADCELL_READ_SAMPLES    10     /* averaged readings per weight    */

void  LoadCell_Init(void);              /* no pin arguments: pins are set in hx711.h */
void  LoadCell_Tare(void);              /* current load becomes zero (keep the cell empty) */
float LoadCell_GetWeightGrams(void);    /* grams; returns 0.0 if the HX711 is not ready    */

#endif // LOADCELL_H
