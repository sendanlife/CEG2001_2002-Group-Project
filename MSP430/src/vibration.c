#include <msp430.h>
#include <stdint.h>
#include "vibration.h"

/* ---- Pin: P2.5 (change these lines to move it) ---- */
#define VIB_DIR          P2DIR
#define VIB_OUT          P2OUT
#define VIB_PIN          BIT5

/* Module logic: 1 = signal HIGH turns the motor ON (typical for the YwRobot module).
   Set to 0 if your module vibrates when the pin is LOW instead.                        */
#define VIB_ACTIVE_HIGH  1

#define VIB_CPU_HZ       1000000UL      /* default DCO ~1 MHz */

static void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(VIB_CPU_HZ / 1000UL);
    }
}

void Vibration_Init(void)
{
    VIB_DIR |= VIB_PIN;                 /* output */
    Vibration_Off();                    /* start with the motor off */
}

void Vibration_On(void)
{
#if VIB_ACTIVE_HIGH
    VIB_OUT |= VIB_PIN;
#else
    VIB_OUT &= ~VIB_PIN;
#endif
}

void Vibration_Off(void)
{
#if VIB_ACTIVE_HIGH
    VIB_OUT &= ~VIB_PIN;
#else
    VIB_OUT |= VIB_PIN;
#endif
}

void Vibration_Pulse(uint16_t on_ms)
{
    Vibration_On();
    delay_ms(on_ms);
    Vibration_Off();
}

/* Flowchart: "Trigger Vibration Motor: Shake Jam" - short bursts shake a stuck pill loose */
void Vibration_Shake(uint8_t cycles, uint16_t on_ms, uint16_t off_ms)
{
    while (cycles--) {
        Vibration_Pulse(on_ms);
        delay_ms(off_ms);
    }
}
