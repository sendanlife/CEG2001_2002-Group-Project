#include "stepper_motor.h"
#include <msp430.h>

static const unsigned char sequence[4] = {0x09,0x0C,0x06,0x03};

static unsigned char phase = 0;
static unsigned char current_slot = 0;

static void motor_delay(void)
{
    volatile unsigned int i;

    for (i = 0; i < 3000; i++)
    {
    }
}

void motor_init(void)
{
    P1SEL &= ~MOTOR_MASK;
    P1DIR |= MOTOR_MASK;
    P1OUT &= ~MOTOR_MASK;

    phase = 0;
    current_slot = 0;
}

void motor_step(unsigned int steps)
{
    unsigned int i;

    for (i = 0; i < steps; i++)
    {
        P1OUT = (P1OUT & ~MOTOR_MASK)
              | sequence[phase];

        motor_delay();

        phase = (phase + 1) % 4;
    }
}

void motor_select_slot(unsigned char target_slot)
{
    unsigned char slots_to_move;

    if (target_slot >= NUMBER_OF_SLOTS)
    {
        return;
    }

    slots_to_move = c(target_slot + NUMBER_OF_SLOTS - current_slot) % NUMBER_OF_SLOTS;

    motor_step((unsigned int)slots_to_move * STEPS_PER_SLOT);

    current_slot = target_slot;
}

unsigned char motor_get_current_slot(void)
{
    return current_slot;
}

void motor_release(void)
{
    P1OUT &= ~MOTOR_MASK;
}