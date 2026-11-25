#ifndef STATUSINDICATOR_H
#define STATUSINDICATOR_H

#define LED_RED_BIT     BIT3    // P2.3 (Fail)
#define LED_YELLOW_BIT  BIT4    // P2.4 (Dispensing)
#define LED_GREEN_BIT   BIT5    // P2.5 (Dispensed)
#define BUZZER_BIT      BIT6    // P2.6 (Completion buzzer)

// Set to 1 for an active buzzer (beeps when HIGH), 0 for a passive buzzer (needs 2.5 kHz tone)
#define BUZZER_IS_ACTIVE 1

void Status_Init(void);
void Status_ClearAll(void);
void Status_SetDispensing(void);
void Status_SetDispensed(void);
void Status_SetFail(void);
void Status_BeepComplete(void);

#endif