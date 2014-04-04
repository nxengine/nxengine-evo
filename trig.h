#ifndef __TRIG_H_
#define __TRIG_H_
#include <stdint.h>
#include "object.h"
extern signed int sin_table[256];
char trig_init(void);
void vector_from_angle(uint8_t angle, int speed, int *xs, int *ys);
int xinertia_from_angle(uint8_t angle, int speed);
int yinertia_from_angle(uint8_t angle, int speed);
uint8_t GetAngle(int curx, int cury, int tgtx, int tgty);
void EmFireAngledShot(Object *o, int objtype, int rand_variance, int speed);
void ThrowObjectAtPlayer(Object *o, int rand_variance, int speed);
void ThrowObject(Object *o, int destx, int desty, int rand_variance, int speed);
void ThrowObjectAtAngle(Object *o, uint8_t angle, int speed);

#endif