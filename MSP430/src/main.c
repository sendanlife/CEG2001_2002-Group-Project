#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"

volatile float latestTemp = 0.0f;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    stepper_init();
    UART_Init();
    TempSensor_Init();
    IRSensor_Init();

    __bis_SR_register(GIE);

    unsigned char sampleCount = 0;
    unsigned long adcSum = 0;

    while (1) {
        if (dispense_flag == 1) {
            dispense_flag = 0;
            IRSensor_Arm();
            stepper_rotate(512);
            __delay_cycles(500000);
            IRSensor_Disarm();

            if (pill_detected_flag == 1) {
                UART_SendString("DISPENSE:OK\n");
            } else {
                UART_SendString("DISPENSE:FAIL\n");
            }
        }

        if (temp_ready_flag == 1) {
            adcSum += rawADC;
            temp_ready_flag = 0;
            sampleCount++;

            if (sampleCount >= 32) {
                unsigned int avgADC = (unsigned int)(adcSum >> 5);
                adcSum = 0;
                sampleCount = 0;

                latestTemp = ((float)avgADC / 4095.0f) * 330.0f;
                unsigned long temp_x10 = ((unsigned long)avgADC * 3300UL) / 4095UL;
                char buf[24];
                sprintf(buf, "TEMP:%lu.%lu\n", temp_x10 / 10, temp_x10 % 10);
                UART_SendString(buf);
            }
        }
    }
    return 0;
}