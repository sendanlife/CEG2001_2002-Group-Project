#ifndef HX711_H_
#define HX711_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

/* ---- Pins (XFW-HX711 module) ----
   HX711 DOUT -> P4.0 (MSP430 input)
   HX711 SCK  -> P4.1 (MSP430 output)
   HX711 VCC  -> 3.3V  (NOT 5V: DOUT would then be 5V and the MSP430 pins are not 5V tolerant)
   HX711 GND  -> GND                                                                        */
#define HX_DIR        P4DIR
#define HX_OUT        P4OUT
#define HX_IN         P4IN
#define HX_DOUT_PIN   BIT0
#define HX_SCK_PIN    BIT1

/* ---- Clock ---- */
#define HX_CPU_HZ     1000000UL        /* default DCO ~1 MHz */

/* ---- Channel / gain: number of SCK pulses per read ----
   25 = channel A, gain 128 (normal for load cells)
   26 = channel B, gain 32
   27 = channel A, gain 64                                    */
#define HX_PULSES     25

void    HX711_Init(void);
bool    HX711_IsReady(void);                              /* DOUT low = conversion ready          */
bool    HX711_ReadRaw(int32_t *value);                    /* one signed 24-bit reading; false = timeout (~1 s) */
bool    HX711_ReadAverage(uint8_t samples, int32_t *avg); /* samples 1..64                        */

bool    HX711_Tare(uint8_t samples);                      /* current load becomes zero            */
void    HX711_SetScale(float counts_per_unit);            /* counts per gram (or any unit)        */
bool    HX711_CalibrateScale(float known_weight, uint8_t samples); /* put known weight on first   */
int32_t HX711_GetOffset(void);
float   HX711_RawToUnits(int32_t raw);                    /* (raw - offset) / scale               */
bool    HX711_GetUnits(uint8_t samples, float *units);    /* averaged weight                      */

void    HX711_PowerDown(void);                            /* SCK high > 60 us = sleep             */
void    HX711_PowerUp(void);                              /* first reading after this takes ~0.4 s */

#endif /* HX711_H_ */
