#ifndef IRSENSOR_H
#define IRSENSOR_H

#define IR_SMALL_BIT      BIT0    // P2.0 (Small dispenser chute)
#define IR_LARGE_BIT      BIT2    // P2.2 (Large dispenser chute)

#define IR_CHANNEL_SMALL  0
#define IR_CHANNEL_LARGE  1

extern volatile unsigned char pill_detected_flag;

void IRSensor_Init(void);
void IRSensor_Arm(unsigned char channel);
void IRSensor_Disarm(void);

#endif