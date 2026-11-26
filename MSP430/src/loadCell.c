#include <msp430.h>
#include "loadCell.h"

#define READ_TIMEOUT       25000UL
#define SATURATION_LEVEL   8388000L     
#define SAME_READINGS      3            

static long hx711_offset = 0;      
static long hx711_last = 0;            
static unsigned char hx711_missing = 0; 

static unsigned char ticksSinceRead = 0;
static unsigned char zeroRun = 0;      
static unsigned char satRun = 0;     

static long avgSum = 0;               
static unsigned char avgCount = 0;

static long filterBuf[FILTER_SIZE];     
static unsigned char filterIdx = 0;
static unsigned char filterCount = 0;
static long filterSum = 0;

void LoadCell_ResetFilter(void) {
    unsigned char i;
    avgSum = 0;
    avgCount = 0;
    filterIdx = 0;
    filterCount = 0;
    filterSum = 0;
    for (i = 0; i < FILTER_SIZE; i++) {
        filterBuf[i] = 0;
    }
}

static void pushAverage(long avg) {
    if (filterCount == FILTER_SIZE) {
        filterSum -= filterBuf[filterIdx];
    } else {
        filterCount++;
    }
    filterBuf[filterIdx] = avg;
    filterSum += avg;
    if (++filterIdx >= FILTER_SIZE) {
        filterIdx = 0;
    }
}

static long filteredRaw(void) {
    return filterCount ? (filterSum / (long)filterCount) : hx711_last;
}

static void noteReading(long v) {
    hx711_last = v;
    hx711_missing = 0;
    ticksSinceRead = 0;

    if (v == 0) {
        if (zeroRun < 255) zeroRun++;
    } else {
        zeroRun = 0;
    }

    if (v >= SATURATION_LEVEL || v <= -SATURATION_LEVEL) {
        if (satRun < 255) satRun++;
    } else {
        satRun = 0;
    }
}

static float countsToGrams(long counts) {
    return (float)counts / HX711_SCALE_FACTOR;
}

static float applyDeadband(float g) {
    if (g > -ZERO_DEADBAND_G && g < ZERO_DEADBAND_G) {
        return 0.0f;
    }
    return g;
}

void LoadCell_Init(void) {
    HX711_DOUT_SEL &= ~HX711_DOUT_BIT;
    HX711_SCK_SEL &= ~HX711_SCK_BIT;

    HX711_DOUT_DIR &= ~HX711_DOUT_BIT;
    HX711_DOUT_REN |= HX711_DOUT_BIT;      
    HX711_DOUT_OUT |= HX711_DOUT_BIT;

    HX711_SCK_DIR |= HX711_SCK_BIT;

    HX711_SCK_OUT |= HX711_SCK_BIT;
    __delay_cycles(100);
    HX711_SCK_OUT &= ~HX711_SCK_BIT;
    __delay_cycles(500000);

    hx711_missing = 0;
    ticksSinceRead = 0;
    zeroRun = 0;
    satRun = 0;
    LoadCell_ResetFilter();
}

unsigned char LoadCell_IsReady(void) {
    return (HX711_DOUT_IN & HX711_DOUT_BIT) == 0;
}

unsigned char LoadCell_IsConnected(void) {
    return !hx711_missing;
}

static unsigned char readRawOnce(long *out) {
    unsigned long timeout = READ_TIMEOUT;
    while (HX711_DOUT_IN & HX711_DOUT_BIT) {
        if (--timeout == 0) {
            hx711_missing = 1;
            return 0;
        }
    }

    unsigned long count = 0;
    unsigned char i;

    for (i = 0; i < 25; i++) {
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

    long value = (long)count;
    if (count & 0x800000UL) {
        value -= 0x1000000L;
    }

    noteReading(value);
    *out = value;
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
        samples = 100;            
    }

    for (i = 0; i < samples; i++) {
        long v;
        if (!readRawOnce(&v)) {
            break;                   
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

    if (good > 0) {                
        hx711_offset = sum / (long)good;
    }
    LoadCell_ResetFilter();
}

void LoadCell_Poll(void) {
    long v;

    if (!LoadCell_IsReady()) {
        return;
    }
    if (!readRawOnce(&v)) {
        return;
    }

    avgSum += v;
    if (++avgCount >= FILTER_READ_SAMPLES) {
        pushAverage(avgSum / (long)avgCount);
        avgSum = 0;
        avgCount = 0;
    }
}

void LoadCell_Tick(void) {
    if (ticksSinceRead < 255) {
        ticksSinceRead++;
    }
}

unsigned char LoadCell_HasWeight(void) {
    return filterCount > 0;
}

long LoadCell_GetRaw(void) {
    return hx711_last;
}

long LoadCell_GetCounts(void) {
    return filteredRaw() - hx711_offset;
}

float LoadCell_GetWeight(void) {
    if (filterCount == 0) {
        return 0.0f;
    }
    return applyDeadband(countsToGrams(LoadCell_GetCounts()));
}

LoadCellStatus LoadCell_GetStatus(void) {
    if (hx711_missing || ticksSinceRead >= NO_DATA_TICKS) {
        return LC_NO_DATA;
    }
    if (zeroRun >= SAME_READINGS) {
        return LC_STUCK_LOW;
    }
    if (satRun >= SAME_READINGS) {
        return LC_SATURATED;
    }
    return LC_OK;
}

const char *LoadCell_StatusText(LoadCellStatus s) {
    switch (s) {
        case LC_NO_DATA:   return "NO_DATA: check HX711 VCC/GND/DOUT";
        case LC_STUCK_LOW: return "STUCK_LOW: check SCK wire / DOUT";
        case LC_SATURATED: return "SATURATED: check E+ E- A+ A- wires";
        default:           return "OK";
    }
}

float LoadCell_RawToGrams(long raw) {
    return applyDeadband(countsToGrams(raw - hx711_offset));
}

float LoadCell_GetGrams(unsigned char samples) {
    return LoadCell_RawToGrams(LoadCell_ReadAverage(samples));
}