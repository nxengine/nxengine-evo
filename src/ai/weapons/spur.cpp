#include "spur.h"

#include "weapons.h"
#include "../../ObjManager.h"
#include "../../sound/sound.h"

#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_SPUR_SHOT, ai_spur_shot);
	ONTICK(OBJ_SPUR_TRAIL, ai_spur_trail);
}

/*
void c------------------------------() {}
*/

// trail l3 11 shot 12
// trail l2 6 damage 8
// trail l1 3 shot 4

void ai_spur_shot(Object *o)
{
	if (IsBlockedInShotDir(o))
	{
		if (!shot_destroy_blocks(o))
		{
			sound(SND_SHOT_HIT);
			shot_spawn_effect(o, EFFECT_STARSOLID);
			shot_spawn_effect(o, EFFECT_SPUR_HIT);
			
			o->Delete();
			return;
		}
	}
	
	Object *enemy;
	if ((enemy = damage_enemies(o)))
	{
		o->shot.damage--;
		
		if (o->shot.damage <= 0 || (enemy->flags & FLAG_INVULNERABLE))
		{
			o->Delete();
			return;
		}
	}
	
	if (--o->shot.ttl < 0)
	{
		shot_spawn_effect(o, EFFECT_STARPOOF);
		o->Delete();
		return;
	}
	
	spur_spawn_trail(o);
}

/*
void c------------------------------() {}
*/


static void spur_spawn_trail(Object *o)
{
static const int trail_sprites[] = \
	{ SPR_TRAIL_SPUR_L1, SPR_TRAIL_SPUR_L2, SPR_TRAIL_SPUR_L3 };
	
static const int trail_damage[] = { 3, 6, 11 };

	// Calculate position to spawn the trail. The shot is 16x8 and the trails are 8x8.
	// We want the trail to cover up the back portion of the shot. The less of it we
	// cover up the more of a gap there will be when we hit a wall. The gap is adjusted
	// so that it does not go too far into the spur_hit circle, but also so that the trail
	// does not start too far back from the firing position.
	int x = o->x, y = o->y;
	
	if (o->state == 0)
	{
		if (o->shot.level != 2) o->state = 1;
		if (o->shot.dir == RIGHT) x += (8 * CSFI);
		if (o->shot.dir == DOWN)  y += (8 * CSFI);
	}
	else
	{
		switch(o->shot.dir)
		{
			case LEFT:  x += (6 * CSFI); break;
			case RIGHT: x += (2 * CSFI); break;
			case UP:    y += (6 * CSFI); break;
			case DOWN:  y += (2 * CSFI); break;
		}
	}
	
	Object *trail = CreateObject(x, y, OBJ_SPUR_TRAIL);
	
	trail->shot.damage = trail_damage[o->shot.level];
	trail->sprite = trail_sprites[o->shot.level];
	
	if ((o->shot.dir == UP || o->shot.dir == DOWN) && o->shot.level != 2)
		trail->sprite++;
}


void ai_spur_trail(Object *o)
{
	if (++o->timer > 20)
	{
		o->frame++;
		if (o->frame >= sprites[o->sprite].nframes)
		{
			o->Delete();
			return;
		}
	}
	
	// keep dealing "damage" even once we are dealing 0 damage,
	// so that the enemy keeps making noise and shaking for as long
	// as it remains in the beam.
	if (damage_enemies(o, FLAG_INVULNERABLE))
	{
		o->shot.damage = 0;
	}
}



