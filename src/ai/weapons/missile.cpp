#include "missile.h"
#include "weapons.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"


#include "../../game.h"
#include "../../player.h"
#include "../../autogen/sprites.h"


#define STATE_WAIT_RECOIL_OVER		1
#define STATE_RECOIL_OVER			2
#define STATE_MISSILE_CAN_EXPLODE	3

struct MissileSettings
{
	int maxspeed;		// max speed of missile
	int accel;			// rate of acceleration
	int smokeamt;		// how much smoke to make on explosion (higher = more)
	int num_booms;		// number of boomflashes to create on impact
	int boomrange;		// max dist away to create the boomflashes
	int damage;			// damage dealt by a direct hit of the missile itself
	int boomdamage;		// damage dealt by contact with a boomflash (AoE damage)
}
missile_settings[] =
{
//  Level 1-3 regular missile
//  have on record here for damage 4, 6, 4; check if that's correct
//  maxspd   acl   smk, nboom, range,  dmg,  bmdmg
	{0xA00,  0x80,  4,   4,		16,		8,	   1},
	{0xA00,  0x100, 6,   6,		32,		15,	   2},
	{0xA00,  0x80,  12,  2,		40,		8,	   2},
	
//  Level 1-3 super missile
//  maxspd   acl   smk, nboom, range,  dmg,  bmdmg
	{0x1400, 0x200, 8,   4,     16,		18,	   1},
	{0x1400, 0x200, 6,   5,     32,		30,	   2},
	{0x1400, 0x200, 12,  3,     40,		18,    2}
};

/*
	Direct Hits:
		L1 Super Missile deals 18 damage, boomflashes are 2 apiece I think
	
	l1 super missile spawns 8 smoke clouds, 2 per frame
	and so it appears that there are 4 booms and the booms are spawning the smoke
	
*/

INITFUNC(AIRoutines)
{
	AFTERMOVE(OBJ_MISSILE_SHOT, ai_missile_shot);
	AFTERMOVE(OBJ_SUPERMISSILE_SHOT, ai_missile_shot);
	AFTERMOVE(OBJ_MISSILE_BOOM_SPAWNER, ai_missile_boom_spawner);
}

/*
void c------------------------------() {}
*/

void ai_missile_shot(Object *o)
{
	int index = o->shot.level + ((o->type == OBJ_SUPERMISSILE_SHOT) ? 3 : 0);
	MissileSettings *settings = &missile_settings[index];
	
	if (o->state == 0)
	{
		o->shot.damage = settings->damage;
		
		if (o->shot.level == 2)
		{
			// initilize wavey effect
			if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
				o->ymark = -0x20;
			else
				o->xmark = -0x20;
			
			// don't let it explode until the "recoil" effect is over.
			o->state = STATE_WAIT_RECOIL_OVER;
			// record position we were fired at (we won't explode until we pass it)
			o->xmark2 = player->x;
			o->ymark2 = player->y;
		}
		else
		{
			o->state = STATE_MISSILE_CAN_EXPLODE;
		}
	}
	
	// accelerate according to current type and level of missile
	// don't use LIMITX here as it can mess up recoil of level 3 super missiles
	switch(o->shot.dir)
	{
		case RIGHT:
			o->xinertia += settings->accel;
			if (o->xinertia > settings->maxspeed) o->xinertia = settings->maxspeed;
		break;
		
		case LEFT:
			o->xinertia -= settings->accel;
			if (o->xinertia < -settings->maxspeed) o->xinertia = -settings->maxspeed;
		break;
		
		case UP:
			o->yinertia -= settings->accel;
			if (o->yinertia < -settings->maxspeed) o->yinertia = -settings->maxspeed;
		break;
		
		case DOWN:
			o->yinertia += settings->accel;
			if (o->yinertia > settings->maxspeed) o->yinertia = settings->maxspeed;
		break;
	}
	
	// wavey effect for level 3
	// (markx/y is used as a "speed" value here)
	if (o->shot.level == 2)
	{
		if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
		{
			o->yinertia += o->ymark;
			
			if (o->ymark > 0 && o->yinertia > 0x100)  o->ymark = -o->ymark;
			if (o->ymark < 0 && o->yinertia < -0x100) o->ymark = -o->ymark;
		}
		else
		{
			o->xinertia += o->xmark;
			
			if (o->xmark > 0 && o->xinertia > 0x100)  o->xmark = -o->xmark;
			if (o->xmark < 0 && o->xinertia < -0x100) o->xmark = -o->xmark;
		}
	}
	
	// check if we hit an enemy
	// level 3 missiles can not blow up while they are "recoiling"
	// what we do is first wait until they're traveling in the direction
	// they're pointing, then wait till they pass the player's original position.
	switch(o->state)
	{
		case STATE_WAIT_RECOIL_OVER:
			switch(o->shot.dir)
			{
				case LEFT: if (o->xinertia <= 0) o->state = STATE_RECOIL_OVER; break;
				case RIGHT: if (o->xinertia >= 0) o->state = STATE_RECOIL_OVER; break;
				case UP: if (o->yinertia <= 0) o->state = STATE_RECOIL_OVER; break;
				case DOWN: if (o->yinertia >= 0) o->state = STATE_RECOIL_OVER; break;
			}
		if (o->state != STATE_RECOIL_OVER)
			break;
			
		case STATE_RECOIL_OVER:
			switch(o->shot.dir)
			{
				case LEFT: if (o->x <= o->xmark2-(2<<CSF)) o->state = STATE_MISSILE_CAN_EXPLODE; break;
				case RIGHT: if (o->x >= o->xmark2+(2<<CSF)) o->state = STATE_MISSILE_CAN_EXPLODE; break;
				case UP: if (o->y <= o->ymark2-(2<<CSF)) o->state = STATE_MISSILE_CAN_EXPLODE; break;
				case DOWN: if (o->y >= o->ymark2+(2<<CSF)) o->state = STATE_MISSILE_CAN_EXPLODE; break;
			}
		if (o->state != STATE_MISSILE_CAN_EXPLODE)
			break;
			
		case STATE_MISSILE_CAN_EXPLODE:
		{
			bool blow_up = false;
			
			if (damage_enemies(o))
			{
				blow_up = true;
			}
			else
			{	// check if we hit a wall
				if (o->shot.dir==LEFT && o->blockl) 	  blow_up = true;
				else if (o->shot.dir==RIGHT && o->blockr) blow_up = true;
				else if (o->shot.dir==UP && o->blocku)	  blow_up = true;
				else if (o->shot.dir==DOWN && o->blockd)  blow_up = true;
			}
			
			if (blow_up)
			{
				sound(SND_MISSILE_HIT);
				
				// create the boom-spawner object for the flashes, smoke, and AoE damage
				Object *sp = CreateObject(o->CenterX(), o->CenterY(), OBJ_MISSILE_BOOM_SPAWNER);
				
				sp->shot.boomspawner.range = settings->boomrange;
				sp->shot.boomspawner.booms_left = settings->num_booms;
				sp->shot.damage = settings->boomdamage;
				
				o->Delete();
				return;
			}
		}
		break;
	}
	
	if (--o->shot.ttl < 0)
		shot_dissipate(o, EFFECT_STARPOOF);
	
	// smoke trails
	if (++o->timer > 2)
	{
		o->timer = 0;
		Caret *trail = effect(o->CenterX() - o->xinertia, \
							  o->CenterY() - o->yinertia, EFFECT_SMOKETRAIL);
		
		const int trailspd = 0x400;
		switch(o->shot.dir)
		{
			case LEFT:	trail->xinertia = trailspd;  trail->y -= (2<<CSF); break;
			case RIGHT: trail->xinertia = -trailspd; trail->y -= (2<<CSF); break;
			case UP:	trail->yinertia = trailspd;  trail->x -= (1<<CSF); break;
			case DOWN:	trail->yinertia = -trailspd; trail->x -= (1<<CSF); break;
		}
	}
	
}

void ai_missile_boom_spawner(Object *o)
{
	if ((++o->timer % 3) != 1)
		return;
	
	if (o->state == 0)
	{
		o->state = 1;
		o->timer = 0;
		
		o->xmark = o->x;
		o->ymark = o->y;
		
		// give us the same bounding box as the boomflash effects
		o->sprite = SPR_BOOMFLASH;
		o->invisible = true;
	}
	
	int range = o->shot.boomspawner.range;
	o->x = o->xmark + (random(-range, range) << CSF);
	o->y = o->ymark + (random(-range, range) << CSF);
	
	effect(o->x, o->y, EFFECT_BOOMFLASH);
	missilehitsmoke(o);
	
	damage_all_enemies_in_bb(o, FLAG_INVULNERABLE);
	
	if (--o->shot.boomspawner.booms_left <= 0)
		o->Delete();
}

static void missilehitsmoke(Object *o)
{
	int smokex = o->CenterX() - (8 << CSF);
	int smokey = o->CenterY() - (8 << CSF);
	Object *smoke;
	
	for(int i=0;i<2;i++)
	{
		smoke = CreateObject(smokex, smokey, OBJ_SMOKE_CLOUD);
		smoke->sprite = SPR_MISSILEHITSMOKE;
		vector_from_angle(random(0,255), random(0x100,0x3ff), &smoke->xinertia, &smoke->yinertia);
	}
}






