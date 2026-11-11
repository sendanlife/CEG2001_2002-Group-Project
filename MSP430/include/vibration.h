#ifndef VIBRATION_H_
#define VIBRATION_H_

#include <stdint.h>

/* YwRobot vibration motor module: S (signal) -> P2.5, VCC -> 3.3V or 5V, GND -> GND
   Module has its own driver transistor, so the MSP430 pin only carries the control signal. */

void Vibration_Init(void);
void Vibration_On(void);
void Vibration_Off(void);
void Vibration_Pulse(uint16_t on_ms);                                    /* on for on_ms, then off       */
void Vibration_Shake(uint8_t cycles, uint16_t on_ms, uint16_t off_ms);   /* jam-clearing burst pattern   */

#endif /* VIBRATION_H_ */
