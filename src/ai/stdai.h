
#ifndef _STDAI_H
#define _STDAI_H

#include "../common/InitList.h"
#include "../nx.h"
#include "../object.h"

extern InitList AIRoutines;

#define ONTICK(OBJTYPE, FUNCTION) objprop[OBJTYPE].ai_routines.ontick = FUNCTION;
#define ONDEATH(OBJTYPE, FUNCTION) objprop[OBJTYPE].ai_routines.ondeath = FUNCTION;
#define AFTERMOVE(OBJTYPE, FUNCTION) objprop[OBJTYPE].ai_routines.aftermove = FUNCTION;
#define ONSPAWN(OBJTYPE, FUNCTION) objprop[OBJTYPE].ai_routines.onspawn = FUNCTION;

#define GENERIC_NPC(O)                                                                                                 \
  {                                                                                                                    \
    ONSPAWN(O, onspawn_generic_npc);                                                                                   \
    ONTICK(O, ai_generic_npc);                                                                                         \
  }

#define GENERIC_NPC_NOFACEPLAYER(O)                                                                                    \
  {                                                                                                                    \
    ONSPAWN(O, onspawn_generic_npc);                                                                                   \
    ONTICK(O, ai_generic_npc_nofaceplayer);                                                                            \
  }

void aftermove_StickToLinkedActionPoint(Object *o);
void onspawn_set_frame_from_id2(Object *o);
void onspawn_snap_to_ground(Object *o);
void ai_generic_angled_shot(Object *o);

void ai_generic_npc(Object *o);
void ai_generic_npc_nofaceplayer(Object *o);
void onspawn_generic_npc(Object *o);

void ai_animate1(Object *o);
void ai_animate2(Object *o);
void ai_animate3(Object *o);
void ai_animate4(Object *o);
void ai_animate5(Object *o);

void KillObjectsOfType(int type);
void DeleteObjectsOfType(int type);

#endif
