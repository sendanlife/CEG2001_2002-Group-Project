#include <msp430.h>
#include <stdio.h>
#include "stepper.h"
#include "msp430uart.h"
#include "tempSensor.h"
#include "irSensor.h"
#include "loadCell.h"

#define USE_STATUS_LEDS 1

#if USE_STATUS_LEDS
#include "statusIndicator.h"
#define LED_INIT()   Status_Init()
#define LED_BUSY()   Status_SetDispensing()   
#define LED_OK()     Status_SetDispensed()  
#define LED_FAIL()   Status_SetFail()        
#define LED_BEEP()   Status_BeepComplete()
#else
#define LED_INIT()
#define LED_BUSY()
#define LED_OK()
#define LED_FAIL()
#define LED_BEEP()
#endif

#define NO_PILL_TIMEOUT_STEPS 2500   
#define LOCKOUT_STEPS         25    
#define SETTLE_DELAY_CYCLES   1000000UL   
#define POST_DISPENSE_SAMPLES 16     
#define BUTTON_BIT            BIT1  
#define SEND_RAW_TEMP         1     
#define LOADCELL_DEBUG        1      

#define LOW_POWER             1
#define LOW_POWER_BITS        LPM0_bits

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

#if LOADCELL_DEBUG
static void sendLoadCellLine(void) {
    char lc[72];
    LoadCellStatus st = LoadCell_GetStatus();
    if (st == LC_OK) {
        sprintf(lc, "LC:OK raw=%ld counts=%ld g*10=%ld\n",
                LoadCell_GetRaw(), LoadCell_GetCounts(), (long)(LoadCell_GetWeight() * 10.0f));
    } else {
        sprintf(lc, "LC:%s raw=%ld\n", LoadCell_StatusText(st), LoadCell_GetRaw());
    }
    UART_SendString(lc);
}
#endif

static unsigned char dispensePills(unsigned char motor, unsigned char expected) {
    unsigned char pills = 0;
    unsigned int sinceProgress = 0;
    unsigned char lockout = 0;

    char run[16];
    sprintf(run, "RUN:%u:%u\n", motor, expected);  
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

    if (pills < expected && lockout == 0) {
        __delay_cycles(500000);
        if (IRSensor_PillDetected(motor)) {
            pills++;
        }
    }

    IRSensor_Disarm(motor);

    char info[32];
    sprintf(info, "DISPENSE:%u:%u:%u\n", motor, pills, expected);
    UART_SendString(info);

    __delay_cycles(SETTLE_DELAY_CYCLES);
    LoadCell_ReadRaw();                         

    float grams = LoadCell_GetGrams(POST_DISPENSE_SAMPLES);   
    if (LoadCell_GetStatus() == LC_OK) {         
        sendWeightGrams(grams);
    }
    LoadCell_ResetFilter();                 

    return (pills >= expected) ? 1 : 0;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    UART_Init();
    TempSensor_Init();
    LoadCell_Init();
    LED_INIT();               
    IRSensor_Init();
    stepper_init();           

    P1DIR &= ~BUTTON_BIT;      
    P1REN |=  BUTTON_BIT;
    P1OUT |=  BUTTON_BIT;

    __bis_SR_register(GIE);

    __delay_cycles(500000);     
    LoadCell_Tare(16);        
    UART_SendString("READY\n");
#if LOADCELL_DEBUG
    sendLoadCellLine();         
#endif

    unsigned char sampleCount = 0;
    unsigned long adcSum = 0;
#if LOADCELL_DEBUG
    unsigned char lcTicks = 0;
#endif

    while (1) {
        if (!(P1IN & BUTTON_BIT)) {
            __delay_cycles(20000);                 
            if (!(P1IN & BUTTON_BIT)) {
                expected1 = 1;
                expected2 = 1;
                dispense1Flag = 1;
                dispense2Flag = 1;
                while (!(P1IN & BUTTON_BIT));     
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
            sprintf(req, "REQ:%u:%u\n", expected1, expected2);  
            UART_SendString(req);

            LED_BUSY();

            while (dispense1Count > 0) {
                dispense1Count--;
                if (!dispensePills(1, expected1)) {
                    allOk = 0;
                }
                __delay_cycles(1000000);      
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
        }

        LoadCell_Poll();

        if (temp_ready_flag == 1) {
            adcSum += rawADC;
            temp_ready_flag = 0;
            sampleCount++;

            LoadCell_Tick();            
#if LOADCELL_DEBUG
            if (++lcTicks >= 6) {     
                lcTicks = 0;
                sendLoadCellLine();
            }
#endif

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
                sprintf(buf, "RAW:%u\n", avgADC);           
                UART_SendString(buf);
#endif
                if (LoadCell_HasWeight() && LoadCell_GetStatus() == LC_OK) {
                    sendWeightGrams(LoadCell_GetWeight());
                }
            }
        }

#if LOW_POWER
        __bic_SR_register(GIE);
        if (!temp_ready_flag && !dispense1Flag && !dispense2Flag && !LoadCell_IsReady()) {
            __bis_SR_register(LOW_POWER_BITS | GIE);
        } else {
            __bis_SR_register(GIE);
        }
#endif
    }
}