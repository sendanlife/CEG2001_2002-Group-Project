#ifndef IRSENSOR_H_
#define IRSENSOR_H_

#include <stdbool.h>

/* IR sensor module: OUT -> P1.3, VCC -> 3.3V, GND -> GND (active LOW when a pill is seen) */

void init_IRSensor(void);
bool check_Pill_Dropped(void);   /* waits up to IR_DETECT_WINDOW_MS for a pill; true = dropped,
                                    green LED on + "Success" sent over UART                 */
void IRSensor_ClearLED(void);    /* turn the green LED off again (call when returning to idle) */

#endif /* IRSENSOR_H_ */
