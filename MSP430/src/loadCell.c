#include <msp430.h>
#include "loadCell.h"

// ~0.3 s of polling: well over one conversion period (10 SPS = 100 ms)
#define READ_TIMEOUT      25000UL
// after a timeout the HX711 is treated as missing; then only a very short look, so a
// disconnected HX711 no longer stalls the program for seconds on every reading
#define MISSING_TIMEOUT     500UL

static long hx711_offset = 0;            // tare value, in raw counts
static long hx711_last = 0;              // last good raw reading
static unsigned char hx711_missing = 0;

static float filterBuf[FILTER_SIZE];
static unsigned char filterIdx = 0;
static unsigned char filterCount = 0;
static float filterSum = 0.0f;

void LoadCell_ResetFilter(void) {
    unsigned char i;
    filterIdx = 0;
    filterCount = 0;
    filterSum = 0.0f;
    for (i = 0; i < FILTER_SIZE; i++) {
        filterBuf[i] = 0.0f;
    }
}

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

    hx711_missing = 0;
    LoadCell_ResetFilter();
}

unsigned char LoadCell_IsReady(void) {
    return (HX711_DOUT_IN & HX711_DOUT_BIT) == 0;
}

unsigned char LoadCell_IsConnected(void) {
    return !hx711_missing;
}

// One reading. Returns 1 and stores it in *out, or 0 if the HX711 did not answer in time.
static unsigned char readRawOnce(long *out) {
    unsigned long timeout = hx711_missing ? MISSING_TIMEOUT : READ_TIMEOUT;
    while (HX711_DOUT_IN & HX711_DOUT_BIT) {
        if (--timeout == 0) {
            hx711_missing = 1;
            return 0;
        }
    }

    unsigned long count = 0;
    unsigned char i;

    // 24 data bits, then a 25th pulse that keeps Channel A / gain 128 for the next reading
    // (the Arduino library does the same with GAIN = 1 extra pulse)
    for (i = 0; i < 25; i++) {
        // SCK must never stay high for more than ~60 us, so no interrupt while it is high.
        // Interrupts stay allowed while SCK is low (UART keeps working).
        // (Same method as the original loadCell.c: this assumes interrupts are enabled when
        //  the load cell is read, which is true everywhere in main.c.)
        __bic_SR_register(GIE);

        HX711_SCK_OUT |= HX711_SCK_BIT;
        __delay_cycles(5);
        if (i < 24) {
            count <<= 1;
            if (HX711_DOUT_IN & HX711_DOUT_BIT) {
                count++;
            }
        }
        HX711_SCK_OUT &= ~HX711_SCK_BIT;

        __bis_SR_register(GIE);
        __delay_cycles(5);
    }

    // 24-bit two's complement -> signed value
    long value = (long)count;
    if (count & 0x800000UL) {
        value -= 0x1000000L;
    }

    *out = value;
    hx711_last = value;
    hx711_missing = 0;
    return 1;
}

long LoadCell_ReadRaw(void) {
    long v;
    return readRawOnce(&v) ? v : hx711_last;
}

long LoadCell_ReadAverage(unsigned char samples) {
    long sum = 0;
    unsigned char good = 0;
    unsigned char i;

    if (samples == 0) {
        samples = 1;
    }
    if (samples > 100) {
        samples = 100;               // keeps the 32-bit sum from overflowing
    }

    for (i = 0; i < samples; i++) {
        long v;
        if (!readRawOnce(&v)) {
            break;                   // HX711 not answering: use what we have
        }
        sum += v;
        good++;
    }

    return good ? (sum / (long)good) : hx711_last;
}

void LoadCell_Tare(unsigned char samples) {
    long sum = 0;
    unsigned char good = 0;
    unsigned char i;

    for (i = 0; i < samples; i++) {
        long v;
        if (!readRawOnce(&v)) {
            break;
        }
        sum += v;
        good++;
    }

    if (good > 0) {                  // if the HX711 never answered, keep the old offset
        hx711_offset = sum / (long)good;
    }
    LoadCell_ResetFilter();
}

static float rawToGrams(long raw) {
    return (float)(raw - hx711_offset) / HX711_SCALE_FACTOR;
}

static float applyDeadband(float g) {
    if (g > -ZERO_DEADBAND_G && g < ZERO_DEADBAND_G) {
        return 0.0f;
    }
    return g;
}

float LoadCell_RawToGrams(long rawAvg) {
    return applyDeadband(rawToGrams(rawAvg));
}

float LoadCell_GetGrams(unsigned char samples) {
    return LoadCell_RawToGrams(LoadCell_ReadAverage(samples));
}

float LoadCell_GetSmoothedGrams(void) {
    return LoadCell_SmoothGrams(LoadCell_ReadAverage(FILTER_READ_SAMPLES));
}

// Moving average over the last FILTER_SIZE averaged readings, then the zero deadband
float LoadCell_SmoothGrams(long rawAvg) {
    float sample = rawToGrams(rawAvg);

    if (filterCount == FILTER_SIZE) {
        filterSum -= filterBuf[filterIdx];
    } else {
        filterCount++;
    }

    filterBuf[filterIdx] = sample;
    filterSum += sample;
    if (++filterIdx >= FILTER_SIZE) {
        filterIdx = 0;
    }

    return applyDeadband(filterSum / (float)filterCount);
}