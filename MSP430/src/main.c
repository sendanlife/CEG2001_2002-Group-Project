#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"

// ---- how many pills each motor should drop per dispense ----
#define EXPECTED_PILLS_1 1
#define EXPECTED_PILLS_2 1

// ---- tuning ----
#define NO_PILL_TIMEOUT_STEPS 2500   // give up after ~30 s (12 ms/step) with no new pill
#define LOCKOUT_STEPS         25     // ~0.3 s ignore time after each pill (debounce)

extern volatile unsigned char dispense1Flag;
extern volatile unsigned char dispense2Flag;
extern volatile unsigned char dispense1Count;
extern volatile unsigned char dispense2Count;
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

    // spin until all expected pills have dropped, or nothing new drops for too long
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
                IRSensor_Arm(motor);          // lockout over: listen again
            }
        } else if (IRSensor_PillDetected(motor)) {
            pills++;
            sinceProgress = 0;
            lockout = LOCKOUT_STEPS;          // ignore this pill's own bounce
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

    char info[24];
    sprintf(info, "PILLS:%u/%u\n", pills, expected);
    UART_SendString(info);

    if (pills == 0) {
        UART_SendString("DISPENSE:FAIL\n");            // nothing dropped
    } else if (pills < expected) {
        UART_SendString("DISPENSE:UNSUCCESSFUL\n");    // not the full amount
    } else {
        UART_SendString("DISPENSE:SUCCESS\n");         // everything dropped
    }

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
            dispensePills(1, EXPECTED_PILLS_1);
            __delay_cycles(1000000);       // ~1 s pause before the next motor
        }

        while (dispense2Count > 0) {
            dispense2Count--;
            dispensePills(2, EXPECTED_PILLS_2);
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