#include "curly_boss.h"
#include "../stdai.h"
#include "../ai.h"
#include "../weapons/weapons.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../player.h"
#include "../../game.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../autogen/sprites.h"


#define CURLYB_FIGHT_START		10
#define CURLYB_WAIT				11
#define CURLYB_WALK_PLAYER		13
#define CURLYB_WALKING_PLAYER	14
#define CURLYB_CHARGE_GUN		20
#define CURLYB_FIRE_GUN			21
#define CURLYB_SHIELD			30

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CURLY_BOSS, ai_curly_boss);
	ONTICK(OBJ_CURLYBOSS_SHOT, ai_curlyboss_shot);
}

/*
void c------------------------------() {}
*/

void ai_curly_boss(Object *o)
{
	switch(o->state)
	{
		case CURLYB_FIGHT_START:
		{
			//o->hp = 12;
			o->state = CURLYB_WAIT;
			o->timer = random(50, 100);
			o->frame = 0;
			if (o->x > player->x) o->dir = LEFT; else o->dir = RIGHT;
			o->flags |= FLAG_SHOOTABLE;
			o->flags &= ~FLAG_INVULNERABLE;
		}
		case CURLYB_WAIT:
			if (o->timer) o->timer--;
			else
			{
				o->state = CURLYB_WALK_PLAYER;
			}
		break;
		
		
		case CURLYB_WALK_PLAYER:
			o->state = CURLYB_WALKING_PLAYER;
			o->frame = 3;
			o->timer = random(50, 100);
			if (o->x > player->x) o->dir = LEFT; else o->dir = RIGHT;
		case CURLYB_WALKING_PLAYER:
			if (++o->animtimer > 2)
			{
				o->animtimer = 0;
				if (++o->frame > 6) o->frame = 3;
			}
			
			XACCEL(0x40);
			
			if (o->timer) o->timer--;
			else
			{
				o->flags |= FLAG_SHOOTABLE;
				o->state = CURLYB_CHARGE_GUN;
				o->timer = 0;
				sound(SND_CHARGE_GUN);
			}
		break;
		
		
		case CURLYB_CHARGE_GUN:
			if (o->x > player->x) o->dir = LEFT; else o->dir = RIGHT;
			
			o->xinertia *= 8;
			o->xinertia /= 9;
			
			if (++o->frame > 1) o->frame = 0;
			if (++o->timer > 50)
			{
				o->state = CURLYB_FIRE_GUN;
				o->frame = 0;
				o->timer = 0;
			}
		break;
		
		case CURLYB_FIRE_GUN:
			o->timer++;
			
			if (!(o->timer & 3))
			{	// time to fire
				
				// check if player is trying to jump over
				if ((abs(o->x - player->x) < (32 * CSFI)) && ((player->y + (10 * CSFI)) < o->y))
				{	// shoot up instead
					o->frame = 2;
					curlyboss_fire(o, UP);
				}
				else
				{
					o->frame = 0;
					curlyboss_fire(o, o->dir);
				}
			}
			
			if (o->timer > 30) o->state = 10;
		break;
		
		case CURLYB_SHIELD:
			if (++o->frame > 8) o->frame = 7;
			if (++o->timer > 30)
			{
				o->frame = 0;
				o->state = CURLYB_FIGHT_START;
			}
		break;
	}


	if (o->state > CURLYB_FIGHT_START && o->state < CURLYB_SHIELD)
	{
		// curly activates her shield anytime a missile's explosion goes off,
		// even if it's nowhere near her at all
		if (sound_is_playing(SND_MISSILE_HIT))
		{
			o->timer = 0;
			o->state = CURLYB_SHIELD;
			o->frame = 7;
			o->flags &= ~FLAG_SHOOTABLE;
			o->flags |= FLAG_INVULNERABLE;
			o->xinertia = 0;
		}
	}
	
	if (o->xinertia > 0x1FF) o->xinertia = 0x1FF;
	if (o->xinertia < -0x1FF) o->xinertia = -0x1FF;
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}

static void curlyboss_fire(Object *o, int dir)
{
	Object *shot = SpawnObjectAtActionPoint(o, OBJ_CURLYBOSS_SHOT);
	
	shot->damage = 6;
	shot->sprite = SPR_SHOT_MGUN_L1;
	shot->dir = o->dir;
	shot->shot.dir = dir;
	
	effect(shot->CenterX(), shot->CenterY(), EFFECT_STARPOOF);
	
	switch(dir)
	{
		case LEFT:
			shot->xinertia = -4096;
			o->apply_xinertia(1 * CSFI);
		break;
		
		case RIGHT:
			shot->xinertia = 4096;
			o->apply_xinertia(-(1 * CSFI));
		break;
		
		case UP:
			shot->yinertia = -4096;
			shot->sprite++;
		break;
	}
	
	sound(SND_POLAR_STAR_L1_2);
}


/*
void c------------------------------() {}
*/

void ai_curlyboss_shot(Object *o)
{
	if (hitdetect(o, player) && !player->hurt_time)
	{
		hurtplayer(o->shot.damage);
	}
	else if (IsBlockedInShotDir(o))
	 { }
	else
	{
		return;
	}
	
	// we get here only if we've hit the player, or the wall
	shot_spawn_effect(o, EFFECT_STARSOLID);
	o->Delete();
}

