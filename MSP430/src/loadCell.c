#include <msp430.h>
#include "loadCell.h"
#include "hx711.h"

void LoadCell_Init(void) {
    HX711_Init();                       /* SCK output, DOUT input */
    HX711_SetScale(LOADCELL_SCALE);     /* Replace with your calibration factor */
}

void LoadCell_Tare(void) {
    HX711_Tare(LOADCELL_TARE_SAMPLES);  /* Average 20 readings */
}

float LoadCell_GetWeightGrams(void) {
    float grams;

    if (HX711_IsReady()) {
        if (HX711_GetUnits(LOADCELL_READ_SAMPLES, &grams))   /* Average 10 readings */
            return grams;
    }
    return 0.0f;                        /* Return 0 if not ready */
}
