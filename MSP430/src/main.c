
#include <msp430.h>
#include <stdint.h>
#include "hx711.h"
#include "msp430uart.h"

/* ---------------- SETTINGS ---------------- */

#define CUP_THRESHOLD_COUNTS  15000L
#define CUP_REMOVAL_THRESHOLD 7500L

#define BASELINE_SAMPLES      40
#define READING_SAMPLES      5

/* LED connected to P1.0 */

/* ---------------- VARIABLES ---------------- */

long baseline_raw = 0;
long current_raw = 0;
long difference = 0;

uint8_t cup_detected = 0;

/* ---------------- UART NUMBER PRINTING ---------------- */

void UART_SendNumber(long number)
{
    char buffer[12];
    int i = 0;

    if (number < 0)
    {
        UART_SendChar('-');
        number = -number;
    }

    if (number == 0)
    {
        UART_SendChar('0');
        return;
    }

    while (number > 0 && i < 11)
    {
        buffer[i++] = (char)('0' + (number % 10));
        number /= 10;
    }

    while (i > 0)
    {
        UART_SendChar(buffer[--i]);
    }
}

/* ---------------- CALCULATE DIFFERENCE ---------------- */

long GetDifference(long value1, long value2)
{
    long result = value1 - value2;

    if (result < 0)
    {
        result = -result;
    }

    return result;
}

/* ---------------- MAIN PROGRAM ---------------- */

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    /* Configure LED */
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    /* Initialise UART */
    UART_Init();

    /* Initialise HX711 */
    HX711_Init();

    __enable_interrupt();

    UART_SendString("\r\n");
    UART_SendString("============================\r\n");
    UART_SendString("   LOAD CELL CUP TEST\r\n");
    UART_SendString("============================\r\n");

    UART_SendString("Keep the platform empty.\r\n");
    UART_SendString("Calibrating baseline...\r\n");

    /* Allow HX711 to settle */
    __delay_cycles(2000000);

    /* Read empty platform baseline */
    if (!HX711_ReadAverage(&baseline_raw, BASELINE_SAMPLES))
    {
        UART_SendString("ERROR: HX711 baseline read failed!\r\n");

        while (1)
        {
            P1OUT ^= BIT0;
            __delay_cycles(500000);
        }
    }

    UART_SendString("Baseline raw reading: ");
    UART_SendNumber(baseline_raw);
    UART_SendString("\r\n");

    UART_SendString("Place your 7g cup on the platform.\r\n");
    UART_SendString("Monitoring started...\r\n\r\n");

    /* ---------------- CONTINUOUS MONITORING ---------------- */

    while (1)
    {
        if (!HX711_ReadAverage(&current_raw, READING_SAMPLES))
        {
            UART_SendString("ERROR: HX711 reading failed!\r\n");
            __delay_cycles(500000);
            continue;
        }

        /* Compare current reading against empty baseline */
        difference = GetDifference(current_raw, baseline_raw);

        UART_SendString("Raw: ");
        UART_SendNumber(current_raw);

        UART_SendString(" | Difference: ");
        UART_SendNumber(difference);

        UART_SendString(" | Cup: ");

        /* Detect cup placement */
        if (!cup_detected && difference >= CUP_THRESHOLD_COUNTS)
        {
            cup_detected = 1;

            P1OUT |= BIT0;

            UART_SendString("DETECTED");
            UART_SendString("\r\n");
        }

        /* Detect cup removal */
        else if (cup_detected &&
                 difference < CUP_REMOVAL_THRESHOLD)
        {
            cup_detected = 0;

            P1OUT &= ~BIT0;

            UART_SendString("REMOVED");
            UART_SendString("\r\n");
        }

        else if (cup_detected)
        {
            UART_SendString("PRESENT");
            UART_SendString("\r\n");
        }
        else
        {
            UART_SendString("NOT DETECTED");
            UART_SendString("\r\n");
        }

        __delay_cycles(1000000);
    }
}
