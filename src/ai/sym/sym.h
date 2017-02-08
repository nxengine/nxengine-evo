#ifndef __AISYM_H_
#define __AISYM_H_
#include "../../object.h"

void ai_null(Object *o);
void ai_hvtrigger(Object *o);
void ai_xp(Object *o);
void ai_powerup(Object *o);
bool Handle_Falling_Left(Object *o);
void ai_hidden_powerup(Object *o);
void ai_xp_capsule(Object *o);
void ai_save_point(Object *o);
void ai_recharge(Object *o);
void ai_chest_closed(Object *o);
void ai_chest_open(Object *o);
void ai_lightning(Object *o);
void ai_teleporter(Object *o);
void ai_door(Object *o);
void ai_largedoor(Object *o);
void ai_press(Object *o);
void ai_terminal(Object *o);
void ai_fan_vert(Object *o);
void ai_fan_hoz(Object *o);
void ai_fan_droplet(Object *o);
void ai_sprinkler(Object *o);
void ai_droplet_spawner(Object *o);
void ai_water_droplet(Object *o);
void ai_bubble_spawner(Object *o);
void ai_chinfish(Object *o);
void ai_fireplace(Object *o);
void ai_straining(Object *o);
void ai_smoke_dropper(Object *o);
void ai_scroll_controller(Object *o);
void ai_quake(Object *o);
void ai_generic_angled_shot(Object *o);
void onspawn_spike_small(Object *o);

#endif
