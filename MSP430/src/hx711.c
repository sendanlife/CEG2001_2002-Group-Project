
#include "hx711.h"

// HX711 connections
// DOUT = P3.6
// SCK  = P3.5

void HX711_Init(void)
{
    // Configure SCK as output
    P3DIR |= BIT5;

    // Configure DOUT as input
    P3DIR &= ~BIT6;

    // Disable GPIO pull resistor on DOUT
    P3REN &= ~BIT6;

    // Start with SCK LOW
    P3OUT &= ~BIT5;

    // Allow HX711 to power up
    __delay_cycles(1000000);
}

uint8_t HX711_ReadRaw(long *result)
{
    unsigned long raw = 0;
    unsigned long timeout = 1000000;
    int i;

    // Wait until HX711 data is ready
    while (P3IN & BIT6)
    {
        if (timeout-- == 0)
        {
            return 0;
        }
    }

    // Read 24 bits from HX711
    for (i = 0; i < 24; i++)
    {
        P3OUT |= BIT5;
        __delay_cycles(1);

        raw <<= 1;

        if (P3IN & BIT6)
        {
            raw |= 1;
        }

        P3OUT &= ~BIT5;
        __delay_cycles(1);
    }

    // One additional pulse selects Channel A, gain 128
    P3OUT |= BIT5;
    __delay_cycles(1);

    P3OUT &= ~BIT5;
    __delay_cycles(1);

    // Convert 24-bit signed value to 32-bit signed value
    if (raw & 0x800000UL)
    {
        raw |= 0xFF000000UL;
    }

    *result = (long)raw;

    return 1;
}

uint8_t HX711_ReadAverage(long *average, uint8_t samples)
{
    long sum = 0;
    long reading;
    uint8_t i;

    if (samples == 0)
    {
        return 0;
    }

    for (i = 0; i < samples; i++)
    {
        if (!HX711_ReadRaw(&reading))
        {
            return 0;
        }

        sum += reading;
    }

    *average = sum / samples;

    return 1;
}
