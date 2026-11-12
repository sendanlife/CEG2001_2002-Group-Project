#include "hx711.h"

static int32_t offset = 0;
static float   scale  = 1.0f;

static void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(HX_CPU_HZ / 1000UL);
    }
}

void HX711_Init(void)
{
    HX_DIR |=  HX_SCK_PIN;              /* SCK = output, low (HX711 awake) */
    HX_OUT &= ~HX_SCK_PIN;
    HX_DIR &= ~HX_DOUT_PIN;             /* DOUT = input                    */
}

bool HX711_IsReady(void)
{
    return !(HX_IN & HX_DOUT_PIN);
}

bool HX711_ReadRaw(int32_t *value)
{
    uint32_t data = 0;
    uint8_t  i;
    uint16_t waited = 0;
    unsigned short gie;

    /* Wait for the HX711 to finish a conversion (~100 ms at 10 SPS) */
    while (!HX711_IsReady()) {
        if (waited++ >= 1000)
            return false;               /* timeout: module not connected / no power */
        delay_ms(1);
    }

    gie = __get_SR_register() & GIE;
    __disable_interrupt();              /* SCK must not stay high > 60 us or the HX711 powers down */

    for (i = 0; i < 24; i++) {          /* 24 data bits, MSB first */
        HX_OUT |= HX_SCK_PIN;
        data <<= 1;
        if (HX_IN & HX_DOUT_PIN)
            data |= 1;
        HX_OUT &= ~HX_SCK_PIN;
    }
    for (i = 24; i < HX_PULSES; i++) {  /* extra pulses select channel/gain of the NEXT reading */
        HX_OUT |= HX_SCK_PIN;
        HX_OUT &= ~HX_SCK_PIN;
    }

    if (gie) __enable_interrupt();

    if (data & 0x800000UL)              /* sign-extend 24-bit two's complement */
        data |= 0xFF000000UL;

    *value = (int32_t)data;
    return true;
}

bool HX711_ReadAverage(uint8_t samples, int32_t *avg)
{
    int32_t sum = 0, v;
    uint8_t i;

    if (samples < 1)  samples = 1;
    if (samples > 64) samples = 64;     /* keeps the 32-bit sum from overflowing */

    for (i = 0; i < samples; i++) {
        if (!HX711_ReadRaw(&v))
            return false;
        sum += v;
    }
    *avg = sum / samples;
    return true;
}

bool HX711_Tare(uint8_t samples)
{
    int32_t avg;
    if (!HX711_ReadAverage(samples, &avg))
        return false;
    offset = avg;
    return true;
}

void HX711_SetScale(float counts_per_unit)
{
    if (counts_per_unit != 0.0f)
        scale = counts_per_unit;
}

bool HX711_CalibrateScale(float known_weight, uint8_t samples)
{
    int32_t avg;
    if (known_weight <= 0.0f || !HX711_ReadAverage(samples, &avg))
        return false;
    scale = (float)(avg - offset) / known_weight;
    if (scale == 0.0f)
        scale = 1.0f;
    return true;
}

int32_t HX711_GetOffset(void)
{
    return offset;
}

float HX711_RawToUnits(int32_t raw)
{
    return (float)(raw - offset) / scale;
}

bool HX711_GetUnits(uint8_t samples, float *units)
{
    int32_t avg;
    if (!HX711_ReadAverage(samples, &avg))
        return false;
    *units = HX711_RawToUnits(avg);
    return true;
}

void HX711_PowerDown(void)
{
    HX_OUT &= ~HX_SCK_PIN;
    HX_OUT |=  HX_SCK_PIN;              /* held high > 60 us -> power down */
    __delay_cycles(100);
}

void HX711_PowerUp(void)
{
    HX_OUT &= ~HX_SCK_PIN;
}