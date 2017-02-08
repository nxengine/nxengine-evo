#ifndef __AIBALLOSPRIEST_H_
#define __AIBALLOSPRIEST_H_
#include "../../object.h"

void ai_ballos_priest(Object *o);
//static void run_flight(Object *o);
//static void spawn_bones(Object *o, int dir);
//static void run_lightning(Object *o);
//static void run_intro(Object *o);
//static void run_defeated(Object *o);
void ai_ballos_target(Object *o);
void ai_ballos_bone_spawner(Object *o);
void ai_ballos_bone(Object *o);

#endif
