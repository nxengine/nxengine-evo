#ifndef __AIMISSILE_H_
#define __AIMISSILE_H_
#include "../../object.h"

void ai_missile_shot(Object *o);
void ai_missile_boom_spawner(Object *o);
void ai_missile_boom_spawner_tick(Object *o);
static void missilehitsmoke(int x, int y, int range);

#endif
