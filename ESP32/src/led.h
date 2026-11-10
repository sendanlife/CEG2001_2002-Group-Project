#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

void LED_Init(void);
void LED_TurnOn(void);
void LED_TurnOff(void);

#ifdef __cplusplus
}
#endif

#endif // LED_H