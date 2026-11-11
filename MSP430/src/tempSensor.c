#include <msp430.h>
#include "tempSensor.h" 

// Global variables scoped to this file
volatile float currentTemp = 0.0;
volatile unsigned int rawADC = 0;

void TempSensor_Init(void) {
    // 1. Configure Hardware Pin (P6.0 as Analog Input)
    P6SEL |= BIT0; 

    // 2. Configure ADC12 Module
    ADC12CTL0 = ADC12SHT0_3 | ADC12ON;      // 64 clock cycles sample time, turn on ADC12
    ADC12CTL1 = ADC12SHP;                   // Use sampling timer
    ADC12CTL2 = ADC12RES;                   // 12-bit resolution (0 to 4095)
    
    // SREF_0: Vcc (3.3V) and Vss (0V). INCH_0: Channel A0 (P6.0)
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0; 
    
    ADC12IE |= BIT0;                        // Enable interrupt for ADC12MEM0

    // 3. Configure Timer0_A3 for periodic background sampling (1 second interval)
    TA0CTL = TASSEL_1 | MC_1;               // ACLK (32kHz), Up Mode
    TA0CCTL0 = CCIE;                        // Enable Timer_A0 interrupt
    TA0CCR0 = 32768;                        // 1 second interval
}

float TempSensor_GetLatestReading(void) {
    return currentTemp;
}

// ---------------------------------------------------------
// Interrupt Service Routines (ISRs)
// ---------------------------------------------------------

// Timer A0 ISR: Fires once every second to trigger a new ADC reading
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {
    ADC12CTL0 |= ADC12ENC + ADC12SC;        // Enable and Start Conversion
}

// ADC12 ISR: Fires automatically when the conversion is complete
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR (void) {
    rawADC = ADC12MEM0; // Extract the raw 12-bit integer (0 to 4095)
    
    // Convert raw 12-bit integer to Celsius for the LM35
    // Math: (rawADC / 4095.0) * 3300mV total range / 10mV step
    currentTemp = ((float)rawADC / 4095.0) * 330.0; 
}