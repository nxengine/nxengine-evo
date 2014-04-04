#ifndef __AISIDEKICKS_H_
#define __AISIDEKICKS_H_
#include "../../object.h"

void ai_misery_frenzied(Object *o);
static Object *fm_spawn_missile(Object *o, int angindex);
void ai_misery_critter(Object *o);
void ai_misery_bat(Object *o);
void ai_misery_missile(Object *o);
void ai_sue_frenzied(Object *o);
static void sue_somersault(Object *o);
static void sue_dash(Object *o);
static void set_ignore_solid(Object *o);
static void sidekick_run_defeated(Object *o, int health);

#endif
