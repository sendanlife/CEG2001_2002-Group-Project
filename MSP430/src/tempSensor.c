#include <msp430.h>
#include "tempSensor.h"

volatile float currentTemp = 0.0f;
volatile unsigned int rawADC = 0;
volatile unsigned char temp_ready_flag = 0;

void TempSensor_Init(void) {
    P6SEL |= BIT0;

    // 2. Configure ADC12 Module
    ADC12CTL0 = ADC12SHT0_3 | ADC12ON;      // 64 clock cycles sample time, turn on ADC12
    ADC12CTL1 = ADC12SHP;                   // Use sampling timer
    ADC12CTL2 = ADC12RES_0;                   // 12-bit resolution (0 to 4095)
    
    // SREF_0: Vcc (3.3V) and Vss (0V). INCH_0: Channel A0 (P6.0)
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0; 
    
    ADC12IE |= BIT0;                        // Enable interrupt for ADC12MEM0

    // 3. Configure Timer0_A3 for periodic background sampling (1 second interval)
    TA0CTL = TASSEL_1 | MC_1;               // ACLK (32kHz), Up Mode
    TA0CCTL0 = CCIE;                        // Enable Timer_A0 interrupt
    TA0CCR0 = 32768;                        // 1 second interval
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
    }
}