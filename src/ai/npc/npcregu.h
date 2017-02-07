#ifndef __AIREGU_H_
#define __AIREGU_H_
#include "../../object.h"

void ai_jenka(Object *o);
void ai_doctor(Object *o);
void ai_toroko(Object *o);
void ai_toroko_teleport_in(Object *o);
void ai_npc_sue(Object *o);
void aftermove_npc_sue(Object *o);
void onspawn_npc_sue(Object *o);
void ai_sue_teleport_in(Object *o);
void ai_king(Object *o);
void ai_blue_robot(Object *o);
void ai_kanpachi_fishing(Object *o);
void ai_professor_booster(Object *o);
void ai_booster_falling(Object *o);
void ai_npc_at_computer(Object *o);
void onspawn_generic_npc(Object *o);
void ai_generic_npc(Object *o);
void ai_generic_npc_nofaceplayer(Object *o);
void npc_generic_walk(Object *o, int basestate);

#endif
