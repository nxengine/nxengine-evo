#ifndef __AIFINMISERY_H_
#define __AIFINMISERY_H_
#include "../../object.h"

void ai_boss_misery(Object *o);
static void run_spells(Object *o);
static void run_teleport(Object *o);
static void run_intro(Object *o);
static void run_defeated(Object *o);
static Object *CreateRing(Object *o, uint8_t angle);
void ai_misery_ring(Object *o);
void aftermove_misery_ring(Object *o);
void ai_misery_phase(Object *o);
void ai_misery_ball(Object *o);
void ai_black_lightning(Object *o);

#endif
