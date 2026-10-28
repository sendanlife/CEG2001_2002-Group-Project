#include <msp430.h>
#include "irSensor.h"
// #include "tempSensor.h" // if you are using your temp sensor too!

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    init_IRSensor();            // Set up the IR sensor & green light

    while (1) {
        check_Pill_Dropped();   // Keep checking if the pill fell!
        __delay_cycles(20000);  // Small rest (~20ms)
    }

    return 0;
}