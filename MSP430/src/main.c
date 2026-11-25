#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"

extern volatile unsigned char dispense1Flag;
extern volatile unsigned char dispense2Flag;
extern volatile unsigned char dispense1Count;
extern volatile unsigned char dispense2Count;
extern volatile unsigned char pill_detected_flag;
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

static void dispenseOnePill(unsigned char motor, unsigned int steps) {
    pill_detected_flag = 0;
    IRSensor_Arm();

    if (motor == 2) {
        stepper2Rotate(steps);
    } else {
        stepper1Rotate(steps);
    }

    __delay_cycles(500000);
    IRSensor_Disarm();

    if (pill_detected_flag == 1) {
        UART_SendString("DISPENSE:OK\n");
    } else {
        UART_SendString("DISPENSE:FAIL\n");
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

    stepper_init();
    UART_Init();
    TempSensor_Init();
    IRSensor_Init();
    LoadCell_Init();

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

        if (dispense2Flag==1) {
            dispense2Flag = 0;
            if (dispense2Count == 0) {
                dispense2Count = 1;
            }
        }

        while (dispense1Count > 0) {
            dispense1Count--;
            dispenseOnePill(1, 600);
        }

        while (dispense2Count > 0) {
            dispense2Count--;
            dispenseOnePill(2, 512);
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