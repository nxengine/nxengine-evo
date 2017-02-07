#ifndef __AIHELL_H_
#define __AIHELL_H_
#include "../../object.h"

void ai_bute_flying(Object *o);
void ai_bute_spawner(Object *o);
void ai_bute_falling(Object *o);
void ai_bute_sword(Object *o);
void ai_bute_archer(Object *o);
void ai_bute_arrow(Object *o);
void ai_bute_dying(Object *o);
static bool run_bute_defeated(Object *o, int hp);
void ai_mesa(Object *o);
void ai_mesa_block(Object *o);
void ai_deleet(Object *o);
void ai_rolling(Object *o);
void ai_statue_base(Object *o);
void ai_statue(Object *o);
void ai_puppy_ghost(Object *o);

#endif
