#ifndef __AIFIREBALL_H_
#define __AIFIREBALL_H_
#include "../../object.h"

void ai_fireball(Object *o);
void ai_fireball_level_23(Object *o);
Object *create_fire_trail(Object *o, int objtype, int level);
void ai_fireball_trail(Object *o);

#endif
