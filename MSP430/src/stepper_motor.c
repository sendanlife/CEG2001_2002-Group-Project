#include "stepper_motor.h"
 
/* Half-step sequence, bit0 = IN1, bit1 = IN2, bit2 = IN3, bit3 = IN4 */
static const uint8_t seq[8] = {
    0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};
 
static uint8_t idx = 0;
 
static void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(CPU_FREQ_HZ / 1000UL);
    }
}
 
static void write_coils(uint8_t pattern)
{
    STEPPER_OUT = (STEPPER_OUT & ~STEPPER_MASK) | ((pattern << STEPPER_SHIFT) & STEPPER_MASK);
}
 
void stepper_init(void)
{
    STEPPER_DIR |= STEPPER_MASK;      /* pins as outputs */
    STEPPER_OUT &= ~STEPPER_MASK;     /* all coils off   */
    idx = 0;
}
 
void stepper_step(stepper_dir_t dir)
{
    if (dir == STEPPER_CW)
        idx = (idx + 1) & 0x07;
    else
        idx = (idx - 1) & 0x07;
 
    write_coils(seq[idx]);
}
 
void stepper_move(uint32_t steps, stepper_dir_t dir, uint16_t step_delay_ms)
{
    while (steps--) {
        stepper_step(dir);
        delay_ms(step_delay_ms);
    }
}
 
void stepper_rotate_degrees(uint16_t degrees, stepper_dir_t dir, uint16_t step_delay_ms)
{
    uint32_t steps = ((uint32_t)degrees * STEPS_PER_REV_HALF) / 360UL;
    stepper_move(steps, dir, step_delay_ms);
}
 
void stepper_stop(void)
{
    write_coils(0x00);                /* avoids heating the motor / ULN2003 */
}
