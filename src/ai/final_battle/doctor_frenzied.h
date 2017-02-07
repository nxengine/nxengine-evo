#ifndef __AIDOCTORFRE_H_
#define __AIDOCTORFRE_H_
#include "../../object.h"

void ai_boss_doctor_frenzied(Object *o);
static void run_jumps(Object *o);
static void run_red_dash(Object *o);
static void run_mega_bats(Object *o);
static void run_teleport(Object *o);
static void run_init(Object *o);
static void run_defeat(Object *o);
static void do_redsplode(Object *o);
static void run_red_drip(Object *o);
void ai_doctor_bat(Object *o);

#endif
