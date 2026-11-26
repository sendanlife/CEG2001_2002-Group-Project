#ifndef STATUSINDICATOR_H
#define STATUSINDICATOR_H

#define LED_RED_BIT     BIT3   
#define LED_YELLOW_BIT  BIT4   
#define LED_GREEN_BIT   BIT5    
#define BUZZER_BIT      BIT6  

#define BUZZER_IS_ACTIVE 1

void Status_Init(void);
void Status_ClearAll(void);
void Status_SetDispensing(void);
void Status_SetDispensed(void);
void Status_SetFail(void);
void Status_BeepComplete(void);

#endif