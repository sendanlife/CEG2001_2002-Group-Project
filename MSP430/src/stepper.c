
#include "stepper.h"

// Motor 1: P1.2, P1.3, P1.4, P1.5
// Motor 2: P6.1, P6.2, P6.3, P6.4

const uint8_t step_sequence[4] = {
    BIT2 | BIT3,
    BIT3 | BIT4,
    BIT4 | BIT5,
    BIT5 | BIT2
};

static int position1 = 0;
static int position2 = 0;

void stepper_init(void)
{
    // Configure Motor 1 pins
    P1DIR |= BIT2 | BIT3 | BIT4 | BIT5;
    P1SEL &= ~(BIT2 | BIT3 | BIT4 | BIT5);
    P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5);

    // Configure Motor 2 pins
    P6DIR |= BIT1 | BIT2 | BIT3 | BIT4;
    P6SEL &= ~(BIT1 | BIT2 | BIT3 | BIT4);
    P6OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4);
}

void steppers_rotate_together(int steps)
{
    int i;
    int direction = (steps >= 0) ? 1 : -1;
    int total_steps = (steps >= 0) ? steps : -steps;

    for (i = 0; i < total_steps; i++)
    {
        // Turn off both motors
        P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5);
        P6OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4);

        // Motor 1: P1.2 - P1.5
        P1OUT |= step_sequence[position1];

        // Motor 2: P6.1 - P6.4
        P6OUT |= (step_sequence[position2] >> 1);

        // Update Motor 1 position
        position1 += direction;

        if (position1 >= 4)
            position1 = 0;
        if (position1 < 0)
            position1 = 3;

        // Update Motor 2 position
        position2 += direction;

        if (position2 >= 4)
            position2 = 0;
        if (position2 < 0)
            position2 = 3;

        // Motor speed
        __delay_cycles(4000);
    }

    // Turn off both motors
    P1OUT &= ~(BIT2 | BIT3 | BIT4 | BIT5);
    P6OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4);
}

// Individual motor functions reserved for later
void stepper_rotate(int steps)
{
}

void stepper2_rotate(int steps)
{
}
