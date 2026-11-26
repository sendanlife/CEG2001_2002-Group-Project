#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"

// 1 = also drive the status LEDs + buzzer from statusIndicator.c (your teammate's code)
// 0 = leave them out (use this if statusIndicator.c / .h are not in your project)
#define USE_STATUS_LEDS 1

#if USE_STATUS_LEDS
#include "statusIndicator.h"
#define LED_INIT()   Status_Init()
#define LED_BUSY()   Status_SetDispensing()   // yellow while a request is running
#define LED_OK()     Status_SetDispensed()    // green when everything dropped
#define LED_FAIL()   Status_SetFail()         // red when anything was missing
#define LED_BEEP()   Status_BeepComplete()
#else
#define LED_INIT()
#define LED_BUSY()
#define LED_OK()
#define LED_FAIL()
#define LED_BEEP()
#endif

// ---- tuning ----
#define NO_PILL_TIMEOUT_STEPS 2500   // give up after ~30 s with no new pill
#define LOCKOUT_STEPS         25     // ~0.3 s ignore time after each pill (debounce)
#define SETTLE_DELAY_CYCLES   1000000UL   // ~1 s for the tray to stop moving before weighing
#define POST_DISPENSE_SAMPLES 16     // load-cell readings averaged after a dispense
#define BUTTON_BIT            BIT1   // P1.1 = S2 button on the LaunchPad (test without the ESP32)
#define SEND_RAW_TEMP         1      // 1 = also send RAW:<adc> with each TEMP line (debug; set 0 once temperature is right)

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

static void sendWeightGrams(float g) {
    if (g < 0.0f) g = 0.0f;
    latestWeight = g;

    unsigned long weight_x10 = (unsigned long)(g * 10.0f + 0.5f);
    char buf[24];
    sprintf(buf, "WEIGHT:%lu.%lu\n", weight_x10 / 10, weight_x10 % 10);
    UART_SendString(buf);
}

// Spins one motor until `expected` pills were seen by its own IR sensor.
// Returns 1 if every expected pill dropped, 0 otherwise.
static unsigned char dispensePills(unsigned char motor, unsigned char expected) {
    unsigned char pills = 0;
    unsigned int sinceProgress = 0;
    unsigned char lockout = 0;

    char run[16];
    sprintf(run, "RUN:%u:%u\n", motor, expected);   // motor is about to start
    UART_SendString(run);

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

    // let vibration die down, then weigh: discard one reading, average the rest
    __delay_cycles(SETTLE_DELAY_CYCLES);
    LoadCell_ReadRaw();                          // throw away the first reading after the wait

    float grams = LoadCell_GetGrams(POST_DISPENSE_SAMPLES);   // average + zero deadband
    if (LoadCell_IsConnected()) {                // nothing is sent if the HX711 is not answering
        sendWeightGrams(grams);
    }
    LoadCell_ResetFilter();                      // the weight just changed: do not blend it with old values

    return (pills >= expected) ? 1 : 0;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    UART_Init();
    TempSensor_Init();
    LoadCell_Init();
    LED_INIT();                 // before IRSensor_Init, so it cannot disturb the IR pull-ups on Port 2
    IRSensor_Init();
    stepper_init();             // last, so nothing overwrites the stepper pins

    P1DIR &= ~BUTTON_BIT;       // S2 button: input with pull-up
    P1REN |=  BUTTON_BIT;
    P1OUT |=  BUTTON_BIT;

    __bis_SR_register(GIE);

    __delay_cycles(500000);     // let the HX711 and temperature sensor supply settle
    LoadCell_Tare(16);          // scale must be empty and still at power-up
    UART_SendString("READY\n"); // ESP32 monitor shows: MSP430 says: READY

    unsigned char sampleCount = 0;
    unsigned long adcSum = 0;
    long weightSum = 0;
    unsigned char weightCount = 0;
    unsigned char haveWeight = 0;

    while (1) {
        // S2 button = same as a card tap with 1 pill per motor
        if (!(P1IN & BUTTON_BIT)) {
            __delay_cycles(20000);                 // debounce
            if (!(P1IN & BUTTON_BIT)) {
                expected1 = 1;
                expected2 = 1;
                dispense1Flag = 1;
                dispense2Flag = 1;
                while (!(P1IN & BUTTON_BIT));      // wait for release
            }
        }

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

        if (dispense1Count > 0 || dispense2Count > 0) {
            unsigned char allOk = 1;

            char req[16];
            sprintf(req, "REQ:%u:%u\n", expected1, expected2);   // request received
            UART_SendString(req);

            LED_BUSY();

            // motor 1 runs first, then motor 2
            while (dispense1Count > 0) {
                dispense1Count--;
                if (!dispensePills(1, expected1)) {
                    allOk = 0;
                }
                __delay_cycles(1000000);       // ~1 s pause before the next motor
            }

            while (dispense2Count > 0) {
                dispense2Count--;
                if (!dispensePills(2, expected2)) {
                    allOk = 0;
                }
            }

            if (allOk) {
                LED_OK();
                LED_BEEP();
            } else {
                LED_FAIL();
            }

            weightSum = 0;                // start the next weight average fresh after a dispense
            weightCount = 0;
        }

        // Weight: take a reading whenever the HX711 has one ready (about every 100 ms),
        // average FILTER_READ_SAMPLES of them, then smooth over the last FILTER_SIZE averages.
        // Nothing waits here, so temperature sampling and the UART are not held up.
        if (LoadCell_IsReady()) {
            weightSum += LoadCell_ReadRaw();
            weightCount++;
            if (weightCount >= FILTER_READ_SAMPLES) {
                latestWeight = LoadCell_SmoothGrams(weightSum / (long)weightCount);
                weightSum = 0;
                weightCount = 0;
                haveWeight = 1;
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

#if SEND_RAW_TEMP
                sprintf(buf, "RAW:%u\n", avgADC);           // raw ADC count: volts = RAW / 4095 * 3.3
                UART_SendString(buf);
#endif

                if (haveWeight) {              // latest smoothed weight (no line if the HX711 is silent)
                    sendWeightGrams(latestWeight);
                }
            }
        }
    }
}