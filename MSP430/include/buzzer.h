#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdint.h>

/* Buzzer on P2.4 (MSP430F5529) - bare 2-pin buzzer (marked "+"), driven through an NPN transistor:

       P2.4 --[1k]-- base of NPN (2N2222 / BC547)
       emitter  -> GND
       collector-> buzzer "-" leg
       buzzer "+" leg -> 5V (or 3V3 if the buzzer is rated for 3.3V)

   Signal HIGH = buzzer ON. Active buzzer = default; for a passive buzzer set BUZZER_PASSIVE to 1
   in buzzer.c (the tone is then made with Timer_A2).                                           */

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint16_t on_ms);                                   /* sound for on_ms, then off */
void Buzzer_Pattern(uint8_t beeps, uint16_t on_ms, uint16_t off_ms);/* e.g. fault alert pattern  */

#endif /* BUZZER_H_ */
