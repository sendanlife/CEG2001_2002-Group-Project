#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Red LED on P2.2 (MSP430F5529): P2.2 -> 330 ohm resistor -> LED anode, LED cathode -> GND */

void LED_Init(void);
void LED_TurnOn(void);
void LED_TurnOff(void);

#ifdef __cplusplus
}
#endif

#endif // LED_H
