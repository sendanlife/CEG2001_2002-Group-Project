#include <msp430.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"
#include "statusIndicator.h"

// ---- tuning ----
#define NO_PILL_TIMEOUT_STEPS 2500   // give up after ~30 s with no new pill
#define LOCKOUT_STEPS         25     // ~0.3 s ignore time after each pill (debounce)

extern volatile unsigned char dispense1Flag;
extern volatile unsigned char dispense2Flag;
extern volatile unsigned char dispense1Count;
extern volatile unsigned char dispense2Count;
extern volatile unsigned char expected1;
extern volatile unsigned char expected2;
extern volatile unsigned char temp_ready_flag;
extern volatile unsigned int rawADC;
#include "statusIndicator.h"

/* ─── TUNABLE SYSTEM CONSTANTS ────────────────────────────────── */
#define STEPS_SMALL_PILL        512     // Adjust to match 1 slot rotation on Small wheel
#define STEPS_LARGE_PILL        512     // Adjust to match 1 slot rotation on Large wheel

#define MIN_WEIGHT_SMALL_G      0.25f   // Minimum weight delta (g) to verify Small pill
#define MIN_WEIGHT_LARGE_G      0.60f   // Minimum weight delta (g) to verify Large pill

#define TEMP_REPORT_TICKS       50      // 50 x 100 ms = 5.0 s between TEMP: packets
#define PICKUP_POLL_MS          500     // Check cup weight every 500 ms after dispense
#define PICKUP_TIMEOUT_MS       60000UL // 60 s window before flagging missed dose

/* ─── LOAD CELL HELPER (WRAPS LoadCell_ReadRaw + LoadCell_RawToGrams) ─── */
static float readAverageWeightGrams(unsigned char samples) {
    if (samples == 0) return 0.0f;
    long sum = 0;
    unsigned char i;
    for (i = 0; i < samples; i++) {
        sum += LoadCell_ReadRaw();
    }
    return LoadCell_RawToGrams(sum / (long)samples);
}

/* ─── INTEGER-BASED FLOAT FORMATTERS (AVOIDS MSP430-GCC PRINTF BLOAT) ─── */
static void appendInt(char *buf, int *idx, long val) {
    char tmp[12];
    int n = 0;
    if (val < 0) {
        buf[(*idx)++] = '-';
        val = -val;
    }
    if (val == 0) {
        buf[(*idx)++] = '0';
        return;
    }
    while (val > 0 && n < 12) {
        tmp[n++] = (char)('0' + (val % 10));
        val /= 10;
    }
    while (n > 0) {
        buf[(*idx)++] = tmp[--n];
    }
}

static void sendTempPacket(float tempC) {
    char msg[24];
    int i = 0;
    long tenths = (long)(tempC * 10.0f + (tempC >= 0.0f ? 0.5f : -0.5f));
    long whole = tenths / 10;
    int frac = (int)(tenths % 10);
    if (frac < 0) frac = -frac;

    msg[i++] = 'T'; msg[i++] = 'E'; msg[i++] = 'M'; msg[i++] = 'P'; msg[i++] = ':';
    appendInt(msg, &i, whole);
    msg[i++] = '.';
    msg[i++] = (char)('0' + frac);
    msg[i++] = '\n';
    msg[i] = '\0';

    UART_SendString(msg);
}

static void dispensePills(unsigned char motor, unsigned char expected) {
    unsigned char pills = 0;
    unsigned int sinceProgress = 0;
    unsigned char lockout = 0;

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
            dispensePills(1, expected1);
            __delay_cycles(1000000);       // ~1 s pause before the next motor
        }

        while (dispense2Count > 0) {
            dispense2Count--;
            dispensePills(2, expected2);
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
static void sendDispensePacket(char sizeCode, unsigned char isOk) {
    char msg[20];
    int i = 0;
    msg[i++] = 'D'; msg[i++] = 'I'; msg[i++] = 'S'; msg[i++] = 'P';
    msg[i++] = 'E'; msg[i++] = 'N'; msg[i++] = 'S'; msg[i++] = 'E';
    msg[i++] = ':'; msg[i++] = sizeCode; msg[i++] = ':';
    if (isOk) {
        msg[i++] = 'O'; msg[i++] = 'K';
    } else {
        msg[i++] = 'F'; msg[i++] = 'A'; msg[i++] = 'I'; msg[i++] = 'L';
    }
    msg[i++] = '\n';
    msg[i] = '\0';

    UART_SendString(msg);
}

static void sendWeightPacket(char sizeCode, float weightG) {
    char msg[24];
    int i = 0;
    if (weightG < 0.0f) weightG = 0.0f;

    long hundredths = (long)(weightG * 100.0f + 0.5f);
    long whole = hundredths / 100;
    int frac = (int)(hundredths % 100);

    msg[i++] = 'W'; msg[i++] = 'E'; msg[i++] = 'I'; msg[i++] = 'G';
    msg[i++] = 'H'; msg[i++] = 'T'; msg[i++] = ':';
    msg[i++] = sizeCode; msg[i++] = ':';
    appendInt(msg, &i, whole);
    msg[i++] = '.';
    msg[i++] = (char)('0' + (frac / 10));
    msg[i++] = (char)('0' + (frac % 10));
    msg[i++] = '\n';
    msg[i] = '\0';

    UART_SendString(msg);
}

static void sendPickupPacket(char sizeCode, unsigned char isTaken) {
    char msg[24];
    int i = 0;
    msg[i++] = 'P'; msg[i++] = 'I'; msg[i++] = 'C'; msg[i++] = 'K';
    msg[i++] = 'U'; msg[i++] = 'P'; msg[i++] = ':';
    msg[i++] = sizeCode; msg[i++] = ':';
    if (isTaken) {
        msg[i++] = 'T'; msg[i++] = 'A'; msg[i++] = 'K'; msg[i++] = 'E'; msg[i++] = 'N';
    } else {
        msg[i++] = 'T'; msg[i++] = 'I'; msg[i++] = 'M'; msg[i++] = 'E';
        msg[i++] = 'O'; msg[i++] = 'U'; msg[i++] = 'T';
    }
    msg[i++] = '\n';
    msg[i] = '\0';

    UART_SendString(msg);
}

/* ─── DISPENSE + DUAL VERIFICATION + PICKUP MONITOR ───────────── */
static void executeDispenseCycle(unsigned char command) {
    unsigned char isLarge   = (command == 2 || command == 'L' || command == 'l') ? 1 : 0;
    char sizeCode           = isLarge ? 'L' : 'S';
    unsigned char irChannel = isLarge ? IR_CHANNEL_LARGE : IR_CHANNEL_SMALL;
    unsigned char motorId   = isLarge ? STEPPER_LARGE : STEPPER_SMALL;
    unsigned int steps      = isLarge ? STEPS_LARGE_PILL : STEPS_SMALL_PILL;
    float minDetectG        = isLarge ? MIN_WEIGHT_LARGE_G : MIN_WEIGHT_SMALL_G;

    /* 1. Yellow LED ON and capture pre-dispense cup weight */
    Status_SetDispensing();
    float baseWeightG = readAverageWeightGrams(4);

    /* 2. Arm ONLY the active dispenser's IR sensor (P2.0 or P2.2) and rotate motor */
    IRSensor_Arm(irChannel);
    stepper_rotate(motorId, steps);
    __delay_cycles(1000000);    // ~1.0 s settle window for pill to finish dropping
    unsigned char irOk = pill_detected_flag;
    IRSensor_Disarm();

    /* 3. Report IR drop status to ESP32 */
    sendDispensePacket(sizeCode, irOk);

    /* 4. Measure post-dispense weight delta and report to ESP32 */
    float postWeightG = readAverageWeightGrams(4);
    float deltaG = postWeightG - baseWeightG;
    if (deltaG < 0.0f) deltaG = 0.0f;
    sendWeightPacket(sizeCode, deltaG);

    /* 5. Verify both IR beam break AND weight increase */
    if (irOk && (deltaG >= minDetectG)) {
        Status_SetDispensed();  // Green LED ON (P2.5)
        Status_BeepComplete();  // Two 150 ms buzzer beeps (P2.6)

        /* 6. Monitor cup pickup (2 consecutive reads <= 50% of added pill weight) */
        float pickupThresholdG = baseWeightG + (deltaG * 0.5f);
        unsigned char lowCount = 0;
        unsigned char pickedUp = 0;
        unsigned long elapsedMs = 0;

        while (elapsedMs < PICKUP_TIMEOUT_MS) {
            __delay_cycles(500000); // 500 ms polling slice at ~1 MHz DCO
            elapsedMs += PICKUP_POLL_MS;

            float currentG = readAverageWeightGrams(2);
            if (currentG <= pickupThresholdG) {
                lowCount++;
                if (lowCount >= 2) {
                    pickedUp = 1;
                    break;
                }
            } else {
                lowCount = 0;
            }
        }

        if (pickedUp) {
            sendPickupPacket(sizeCode, 1);
            Status_ClearAll();  // Turn off Green LED once dose is collected
        } else {
            sendPickupPacket(sizeCode, 0);
            Status_SetFail();   // Red LED ON (P2.3) for missed-dose timeout
        }
    } else {
        /* Jam, empty chute, or underweight drop */
        Status_SetFail();       // Red LED ON (P2.3)
    }
}

/* ─── MAIN ENTRY POINT ────────────────────────────────────────── */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    /* Initialize all peripheral drivers */
    UART_Init();                // 9600-baud USCI_A0 (P3.3 TX / P3.4 RX)
    stepper_init();             // Small (P1.2–P1.5) & Large (P6.1–P6.4) steppers
    TempSensor_Init();          // LM35 on P6.0 (ADC12 + Timer0_A3)
    IRSensor_Init();            // Dual IR sensors: Small (P2.0) & Large (P2.2)
    LoadCell_Init();            // HX711 24-bit ADC
    Status_Init();              // LEDs (P2.3–P2.5) & Buzzer (P2.6)

    __enable_interrupt();       // Enable global interrupts (GIE = 1)

    __delay_cycles(500000);     // Allow HX711 and LM35 rails to stabilize
    LoadCell_Tare(16);          // Zero the empty collection cup on boot

    unsigned int loopTicks = 0;

    while (1) {
        /* Check for Small (1 / 'S' / 'D') or Large (2 / 'L') dispense commands */
        if (dispense_flag != 0) {
            unsigned char cmd = dispense_flag;
            dispense_flag = 0;
            executeDispenseCycle(cmd);
            loopTicks = 0;
        }

        /* Stream temperature telemetry every ~5.0 seconds */
        if (loopTicks >= TEMP_REPORT_TICKS) {
            float tempC = TempSensor_GetLatestReading();
            sendTempPacket(tempC);
            loopTicks = 0;
        }

        __delay_cycles(100000); // 100 ms idle slice keeps UART trigger responsive
        loopTicks++;
    }
}