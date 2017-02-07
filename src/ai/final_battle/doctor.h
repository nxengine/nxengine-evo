#ifndef _DOCTOR_H
#define _DOCTOR_H
#include "../../object.h"

extern int crystal_xmark, crystal_ymark;
extern bool crystal_tofront;

void ai_boss_doctor(Object *o);
void ai_doctor_shot(Object *o);
void ai_doctor_shot_trail(Object *o);
void ai_doctor_blast(Object *o);
void aftermove_red_crystal(Object *o);
void ai_doctor_crowned(Object *o);

#endif
