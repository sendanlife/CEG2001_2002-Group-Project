#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"

// ---- tuning ----
#define NO_PILL_TIMEOUT_STEPS 2500   // give up after ~30 s with no new pill
#define LOCKOUT_STEPS         25     // ~0.3 s ignore time after each pill (debounce)

extern volatile unsigned char dispense1Flag;
extern volatile unsigned char dispense2Flag;
extern volatile unsigned char dispense1Count;
extern volatile unsigned char dispense2Count;
extern volatile unsigned char expected1;
extern volatile unsigned char expected2;
extern volatile unsigned char temp_ready_flag;
extern volatile unsigned int rawADC;

volatile float latestTemp = 0.0f;
volatile float latestWeight = 0.0f;

static void sendWeightReading(long rawAvg) {
    latestWeight = LoadCell_RawToGrams(rawAvg);
    unsigned long weight_x10 = (unsigned long)(latestWeight * 10.0f + 0.5f);
    char buf[24];
    sprintf(buf, "WEIGHT:%lu.%lu\n", weight_x10 / 10, weight_x10 % 10);
    UART_SendString(buf);
}

static void dispensePills(unsigned char motor, unsigned char expected) {
    unsigned char pills = 0;
    unsigned int sinceProgress = 0;
    unsigned char lockout = 0;

    IRSensor_Arm(motor);

    while (pills < expected && sinceProgress < NO_PILL_TIMEOUT_STEPS) {
        if (motor == 2) {
            stepper2Step();
        } else {
            stepper1Step();
        }
        sinceProgress++;

        if (lockout > 0) {
            lockout--;
            if (lockout == 0) {
                IRSensor_Arm(motor);
            }
        } else if (IRSensor_PillDetected(motor)) {
            pills++;
            sinceProgress = 0;
            lockout = LOCKOUT_STEPS;
        }
    }

    if (motor == 2) {
        stepper2Release();
    } else {
        stepper1Release();
    }

    // a pill still falling after the motor stopped
    if (pills < expected && lockout == 0) {
        __delay_cycles(500000);
        if (IRSensor_PillDetected(motor)) {
            pills++;
        }
    }

    IRSensor_Disarm(motor);

    // DISPENSE:<motor>:<dropped>:<expected>  (Flask decides success/unsuccessful/failed)
    char info[32];
    sprintf(info, "DISPENSE:%u:%u:%u\n", motor, pills, expected);
    UART_SendString(info);

    long postSum = 0;
    unsigned char i;
    for (i = 0; i < 4; i++) {
        postSum += LoadCell_ReadRaw();
    }
    sendWeightReading(postSum / 4);
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    UART_Init();
    TempSensor_Init();
    IRSensor_Init();
    LoadCell_Init();
    stepper_init();             // last, so nothing overwrites the stepper pins

    __bis_SR_register(GIE);

    LoadCell_Tare(16);

    unsigned char sampleCount = 0;
    unsigned long adcSum = 0;
    long weightSum = 0;
    unsigned char weightCount = 0;

    while (1) {
        if (dispense1Flag == 1) {
            dispense1Flag = 0;
            if (dispense1Count == 0) {
                dispense1Count = 1;
            }
        }

        if (dispense2Flag == 1) {
            dispense2Flag = 0;
            if (dispense2Count == 0) {
                dispense2Count = 1;
            }
        }

        // motor 1 runs first, then motor 2
        while (dispense1Count > 0) {
            dispense1Count--;
            dispensePills(1, expected1);
            __delay_cycles(1000000);       // ~1 s pause before the next motor
        }

        while (dispense2Count > 0) {
            dispense2Count--;
            dispensePills(2, expected2);
        }

        if (temp_ready_flag == 1) {
            adcSum += rawADC;
            temp_ready_flag = 0;
            sampleCount++;

            if ((sampleCount & 0x03) == 0 && LoadCell_IsReady()) {
                weightSum += LoadCell_ReadRaw();
                weightCount++;
            }

            if (sampleCount >= 32) {
                unsigned int avgADC = (unsigned int)(adcSum >> 5);
                adcSum = 0;
                sampleCount = 0;

                latestTemp = ((float)avgADC / 4095.0f) * 330.0f;
                unsigned long temp_x10 = ((unsigned long)avgADC * 3300UL) / 4095UL;
                char buf[24];
                sprintf(buf, "TEMP:%lu.%lu\n", temp_x10 / 10, temp_x10 % 10);
                UART_SendString(buf);

                if (weightCount > 0) {
                    sendWeightReading(weightSum / (long)weightCount);
                    weightSum = 0;
                    weightCount = 0;
                }
            }
        }
    }
}