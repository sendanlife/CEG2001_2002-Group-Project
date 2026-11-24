#include <msp430.h>
#include "loadCell.h"

static long hx711_offset = 0;

void LoadCell_Init(void) {
    HX711_DOUT_SEL &= ~HX711_DOUT_BIT;
    HX711_SCK_SEL &= ~HX711_SCK_BIT;

    HX711_DOUT_DIR &= ~HX711_DOUT_BIT;
    HX711_DOUT_REN |= HX711_DOUT_BIT;
    HX711_DOUT_OUT |= HX711_DOUT_BIT;

    HX711_SCK_DIR |= HX711_SCK_BIT;

    // Power-cycle reset the HX711 (>60 us HIGH) and let the ADC settle
    HX711_SCK_OUT |= HX711_SCK_BIT;
    __delay_cycles(100);
    HX711_SCK_OUT &= ~HX711_SCK_BIT;
    __delay_cycles(500000);
}

unsigned char LoadCell_IsReady(void) {
    return (HX711_DOUT_IN & HX711_DOUT_BIT) == 0;
}

long LoadCell_ReadRaw(void) {
    unsigned long timeout = 100000UL;
    while (HX711_DOUT_IN & HX711_DOUT_BIT) {
        if (--timeout == 0) {
            return hx711_offset;
        }
    }

    unsigned long count = 0;
    unsigned char i;
    for (i = 0; i < 24; i++) {
        __bic_SR_register(GIE);
        HX711_SCK_OUT |= HX711_SCK_BIT;
        __delay_cycles(5);
        count <<= 1;
        if (HX711_DOUT_IN & HX711_DOUT_BIT) {
            count++;
        }
        HX711_SCK_OUT &= ~HX711_SCK_BIT;
        __bis_SR_register(GIE);
        __delay_cycles(5);
    }

    // 25th pulse selects Channel A, Gain 128 for the next conversion
    __bic_SR_register(GIE);
    HX711_SCK_OUT |= HX711_SCK_BIT;
    __delay_cycles(5);
    HX711_SCK_OUT &= ~HX711_SCK_BIT;
    __bis_SR_register(GIE);

    if (count & 0x800000UL) {
        count |= 0xFF000000UL;
    }
    return (long)count;
}

void LoadCell_Tare(unsigned char samples) {
    if (samples == 0) {
        return;
    }
    long sum = 0;
    unsigned char i;
    for (i = 0; i < samples; i++) {
        sum += LoadCell_ReadRaw();
    }
    hx711_offset = sum / (long)samples;
}

float LoadCell_RawToGrams(long rawAvg) {
    float grams = (float)(rawAvg - hx711_offset) / HX711_SCALE_FACTOR;
    if (grams < 0.0f) {
        grams = 0.0f;
    }
    return grams;
}