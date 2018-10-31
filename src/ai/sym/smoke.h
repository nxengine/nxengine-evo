#ifndef __SMOKE_H_
#define __SMOKE_H_
#include "../../object.h"
Object *SmokePuff(int x, int y);
void SmokeClouds(Object *o, int nclouds, int rangex = 0, int rangey = 0, Object *pushbehind = NULL);
void SmokeXY(int x, int y, int nclouds, int rangex = 0, int rangey = 0, Object *pushbehind = NULL);
void SmokeSide(Object *o, int nclouds, int dir);
void SmokeCloudsSlow(int x, int y, int nclouds);
void SmokeBoomUp(Object *o);
void ai_smokecloud(Object *o);

#endif