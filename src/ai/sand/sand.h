#ifndef __AISAND_H_
#define __AISAND_H_
#include "../../object.h"

void ai_polish(Object *o);
void ondeath_polish(Object *o);
void ai_polishbaby(Object *o);
void ai_sandcroc(Object *o);
void ai_sunstone(Object *o);
void ai_crow(Object *o);
void ai_crowwithskull(Object *o);
void ai_skullhead(Object *o);
void ai_skullhead_carried(Object *o);
void aftermove_skullhead_carried(Object *o);
void ai_skeleton_shot(Object *o);
void ai_armadillo(Object *o);
void ai_skullstep(Object *o);
void ai_skullstep_foot(Object *o);
void skullstep_do_step(Object *o, Object *skull, int angle);
void ai_skeleton(Object *o);
void ai_curlys_mimigas(Object *o);
void ai_beetle_horizwait(Object *o);

#endif