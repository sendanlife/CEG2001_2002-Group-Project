#include <msp430.h>
#include "tempSensor.h"

volatile float currentTemp = 0.0f;
volatile unsigned int rawADC = 0;
volatile unsigned char temp_ready_flag = 0;

void TempSensor_Init(void) {
    P6SEL |= BIT0;

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 = ADC12SHT0_3 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 = ADC12RES_2;
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;
    ADC12IE = ADC12IE0;

    TA0CTL = TASSEL_1 | MC_1 | TACLR;
    TA0CCTL0 = CCIE;
    TA0CCR0 = 5119;
}

float TempSensor_GetLatestReading(void) {
    unsigned int adcSnapshot = rawADC;
    currentTemp = ((float)adcSnapshot / 4095.0f) * 330.0f;
    return currentTemp;
}

void __attribute__((interrupt(TIMER0_A0_VECTOR))) Timer_A(void) {
    ADC12CTL0 |= ADC12ENC | ADC12SC;
}

void __attribute__((interrupt(ADC12_VECTOR))) ADC12_ISR(void) {
    if (ADC12IV == ADC12IV_ADC12IFG0) {
        rawADC = ADC12MEM0;
        temp_ready_flag = 1;
        __bic_SR_register_on_exit(LPM3_bits);  
    }
}