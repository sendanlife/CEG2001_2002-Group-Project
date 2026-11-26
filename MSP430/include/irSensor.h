#ifndef IRSENSOR_H
#define IRSENSOR_H

<<<<<<< HEAD
#define IR_SMALL_BIT      BIT0    // P2.0 (Small dispenser chute)
#define IR_LARGE_BIT      BIT2    // P2.2 (Large dispenser chute)

#define IR_CHANNEL_SMALL  0
#define IR_CHANNEL_LARGE  1

extern volatile unsigned char pill_detected_flag;

void IRSensor_Init(void);
void IRSensor_Arm(unsigned char channel);
void IRSensor_Disarm(void);
=======
extern volatile unsigned char pill_detected_flag1;   // sensor on P2.0 (stepper 1)
extern volatile unsigned char pill_detected_flag2;   // sensor on P2.2 (stepper 2)

void IRSensor_Init(void);
void IRSensor_Arm(unsigned char motor);       // motor = 1 or 2
void IRSensor_Disarm(unsigned char motor);    // motor = 1 or 2
unsigned char IRSensor_PillDetected(unsigned char motor);
>>>>>>> 4ae0d6afe5619732ed3bf239f7bd9b3ef93233b9

#endif