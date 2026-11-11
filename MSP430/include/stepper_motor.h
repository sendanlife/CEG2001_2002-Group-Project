#ifndef STEPPER_H_
#define STEPPER_H_

#include <msp430.h>
#include <stdint.h>

/* ---- Pin configuration: P3.0..P3.3 -> ULN2003 IN1..IN4 ---- */
#define STEPPER_OUT    P3OUT
#define STEPPER_DIR    P3DIR
#define STEPPER_MASK   (BIT0 | BIT1 | BIT2 | BIT3)
#define STEPPER_SHIFT  0              /* 0 when using Px.0-Px.3, 4 when using Px.4-Px.7 */

/* ---- Timing ---- */
#define CPU_FREQ_HZ    1000000UL      /* default DCO ~1 MHz; change if you set the clock */

/* ---- 28BYJ-48 constants ---- */
#define STEPS_PER_REV_HALF  4096UL    /* half-step mode (8-step sequence)  */
#define STEPS_PER_REV_FULL  2048UL    /* full-step mode (4-step sequence)  */

typedef enum { STEPPER_CW = 0, STEPPER_CCW = 1 } stepper_dir_t;

void stepper_init(void);
void stepper_step(stepper_dir_t dir);                                   /* one half-step */
void stepper_move(uint32_t steps, stepper_dir_t dir, uint16_t delay_ms);/* N half-steps  */
void stepper_rotate_degrees(uint16_t degrees, stepper_dir_t dir, uint16_t delay_ms);
void stepper_stop(void);                                                /* coils off     */

#endif /* STEPPER_H_ */