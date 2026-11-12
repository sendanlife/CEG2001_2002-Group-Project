#include "loadCell.h"
#include <Arduino.h>
#include "HX711.h"

// Keep the HX711 object private to this file (Encapsulation)
static HX711 scale;

void LoadCell_Init(int dout_pin, int sck_pin) {
    scale.begin(dout_pin, sck_pin);
    scale.set_scale(2280.0); // Replace with your calibration factor
}

void LoadCell_Tare(void) {
    scale.tare(20); // Average 20 readings
}

float LoadCell_GetWeightGrams(void) {
    if (scale.is_ready()) {
        return scale.get_units(10); // Average 10 readings
    }
    return 0.0; // Return 0 if not ready
}