#include <msp430.h>
#include "tempSensor.h" 

// Global variables scoped to this file
volatile float currentTemp = 0.0;
volatile unsigned int rawADC = 0;

void TempSensor_Init(void) {
    // 1. Configure Hardware Pin
    ADC10AE0 |= BIT0; // Enable analog input on Pin P1.0 (Channel 0)

    // 2. Configure ADC10 Module
    // SREF_0: Use Vcc (3.3V) and Vss (0V) as voltage references
    // ADC10SHT_3: 64 ADC10CLK cycles for sample-and-hold time (allows voltage to stabilize)
    // ADC10ON: Turn on the ADC10 peripheral
    // ADC10IE: Enable ADC interrupt so we don't have to poll it
    ADC10CTL0 = SREF_0 | ADC10SHT_3 | ADC10ON | ADC10IE;
    
    // INCH_0: Select Input Channel 0 (P1.0)
    // ADC10DIV_3: Divide ADC10 clock by 4
    ADC10CTL1 = INCH_0 | ADC10DIV_3; 
    
    // 3. Configure Timer_A for periodic background sampling
    // TASSEL_1: Use ACLK (Auxiliary Clock, typically 32kHz crystal or internal VLO)
    // MC_1: Up mode (counts up to CCR0 then resets)
    TACTL = TASSEL_1 | MC_1;
    CCTL0 = CCIE;     // Enable Timer_A0 interrupt
    CCR0 = 32768;     // Set interval to 1 second (assuming 32kHz ACLK)
}

float TempSensor_GetLatestReading(void) {
    // Returns the most recently calculated temperature to main.c
    return currentTemp;
}

// ---------------------------------------------------------
// Interrupt Service Routines (ISRs)
// ---------------------------------------------------------

// Timer A0 ISR: Fires once every second to trigger a new ADC reading
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {
    ADC10CTL0 |= ENC + ADC10SC; // Enable Conversion (ENC) and Start Conversion (ADC10SC)
}

// ADC10 ISR: Fires automatically when the analog-to-digital conversion is complete
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void) {
    rawADC = ADC10MEM; // Extract the raw 10-bit integer (0 to 1023)
    
    // Convert raw integer to Celsius. 
    // This formula assumes an LM35 sensor (10mV / Degree C) and a 3.3V reference.
    // Math: (rawADC / 1023.0) * 3300mV total range / 10mV step
    currentTemp = ((float)rawADC / 1023.0) * 330.0; 
}