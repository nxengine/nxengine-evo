#include "blade.h"
#include "../ai.h"
#include "weapons.h"
#include "../../ObjManager.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../p_arms.h"


// how far away the area-of-effect slashes are spawned when
// the blade hits something and pauses for a moment dealing extra damage.
#define BLADE_AOE		(64 * CSFI)

#define STATE_FLYING	0
#define STATE_AOE		1


INITFUNC(AIRoutines)
{
	AFTERMOVE(OBJ_BLADE12_SHOT, aftermove_blade_l12_shot);
	ONTICK(OBJ_BLADE3_SHOT, ai_blade_l3_shot);
	
	AFTERMOVE(OBJ_BLADE_SLASH, aftermove_blade_slash);
}

/*
void c------------------------------() {}
*/

void ai_blade_l3_shot(Object *o)
{
	
	switch(o->state)
	{
		case STATE_FLYING:
		{
			if ((++o->timer % 4) == 1)
			{
				Object *slash = CreateObject(o->x, o->y - (12 * CSFI), OBJ_BLADE_SLASH);
				
				if (++o->timer2 & 1)
				{
					slash->dir = LEFT;
					slash->x += (10 * CSFI);
				}
				else
				{
					slash->dir = RIGHT;
					slash->x -= (10 * CSFI);
				}
				
				sound(SND_SLASH);
			}
			
			if (++o->timer2 > o->shot.ttl)
			{
				shot_dissipate(o);
				return;
			}
			
			// damage enemies and hit walls
			if (o->timer2 >= 5)
			{
				Object *enemy;
				if ((enemy = damage_enemies(o)))
				{
					if (enemy->flags & FLAG_INVULNERABLE)
					{
						shot_spawn_effect(o, EFFECT_STARSOLID);
						sound(SND_SHOT_HIT);
						o->Delete();
					}
					else
					{
						o->x += o->xinertia;
						o->y += o->yinertia;
						o->xinertia = 0;
						o->yinertia = 0;
						
						o->state = STATE_AOE;
						o->frame = 1;
						o->timer = 0;
					}
				}
				else if (IsBlockedInShotDir(o))
				{
					if (!shot_destroy_blocks(o))
						sound(SND_SHOT_HIT);
					
					shot_spawn_effect(o, EFFECT_STARSOLID);
					o->Delete();
				}
			}
		}
		break;
		
		case STATE_AOE:
		{
			if (!random(0, 2))
			{
				Object *slash = CreateObject(o->x + random(-BLADE_AOE, BLADE_AOE),
											 o->y + random(-BLADE_AOE, BLADE_AOE),
											 OBJ_BLADE_SLASH);
				
				slash->dir = random(0, 1) ? LEFT : RIGHT;
				sound(SND_SLASH);
			}
			
			if (++o->timer > 50)
				o->Delete();
		}
		break;
	}
	
	o->invisible = (o->timer & 1);
}

void aftermove_blade_slash(Object *o)
{
	ANIMATE_FWD(2);
	if (o->frame >= 4)
	{
		o->Delete();
		return;
	}
	
	o->x += (o->dir == LEFT) ? -0x400 : 0x400;
	o->y += 0x400;
	
	static const int damage_for_frames[] = { 0, 1, 2, 2, 2 };
	o->shot.damage = damage_for_frames[o->frame];
	
	// deal damage to anything we touch.
	Object *enemy = damage_enemies(o);
	if (enemy && (enemy->flags & FLAG_INVULNERABLE))
		o->Delete();
}


/*
void c------------------------------() {}
*/

void aftermove_blade_l12_shot(Object *o)
{
	int level = (o->shot.btype - B_BLADE_L1);
	ANIMATE(1, 0, 3);
	
	if (--o->shot.ttl < 0)
	{
		shot_dissipate(o);
		return;
	}
	
	// only start damaging enemies after we've passed the player
	// as it starts slightly behind him
	if (++o->timer >= 4)
	{
		Object *enemy;
		if ((enemy = damage_enemies(o)))
		{
			// on level 2 we can deal damage up to 3 times (18 max)
			if (level == 0 || \
				++o->timer2 >= 3 || (enemy->flags & FLAG_INVULNERABLE))
			{
				o->Delete();
				return;
			}
		}
		else if (IsBlockedInShotDir(o))
		{
			if (!shot_destroy_blocks(o))
				sound(SND_SHOT_HIT);
			
			shot_dissipate(o, EFFECT_STARSOLID);
			return;
		}
	}
	
	switch(level)
	{
		case 0:
			if ((o->timer % 5) == 1)
				sound(SND_FIREBALL);
		break;
		
		case 1:
			if ((o->timer % 7) == 1)
				sound(SND_SLASH);
		break;
	}
}

