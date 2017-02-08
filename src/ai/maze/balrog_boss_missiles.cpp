#include "balrog_boss_missiles.h"

#include "../stdai.h"
#include "../ai.h"

#include "../sym/smoke.h"
#include "../balrog_common.h"
#include "../../game.h"
#include "../../caret.h"
#include "../../player.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#define STATE_CHARGE			10
#define STATE_JUMP_FIRE			20
#define STATE_PAUSE				30
#define STATE_CAUGHT_PLAYER		40


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BALROG_BOSS_MISSILES, ai_balrog_boss_missiles);
	ONDEATH(OBJ_BALROG_BOSS_MISSILES, ondeath_balrog_boss_missiles);
	
	ONTICK(OBJ_BALROG_MISSILE, ai_balrog_missile);
}

/*
void c------------------------------() {}
*/

void ai_balrog_boss_missiles(Object *o)
{
	// try to catch player
	switch(o->state)
	{
		case STATE_CHARGE+1:
		case STATE_JUMP_FIRE+1:
		{
			if (pdistlx(12 * CSFI) && pdistly2(12 * CSFI, 8 * CSFI))
			{
				balrog_grab_player(o);
				hurtplayer(5);
				o->state = STATE_CAUGHT_PLAYER;
			}
		}
		break;
	}
	
	// main state engine
	switch(o->state)
	{
		case 0:
		{
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			FACEPLAYER;
			
			o->state = 1;
			o->frame = 0;
			o->timer = 0;
		}
		case 1:
		{
			if (++o->timer > 30)
			{
				o->state = STATE_CHARGE;
				o->timer2 ^= 1;	// affects how we react if we miss the player
			}
		}
		break;
		
		// charge the player
		case STATE_CHARGE:
		{
			o->timer = 0;
			o->frame = 9;
			o->animtimer = 0;
			o->state++;
		}
		case STATE_CHARGE+1:
		{
			XACCEL(0x20);
			walking_animation(o);
			
			// stuck against the wall?
			if ((o->dir == LEFT && o->blockl) || \
				(o->dir == RIGHT && o->blockr))
			{
				if (++o->timer3 > 5)
					o->state = STATE_JUMP_FIRE;
			}
			else
			{
				o->timer3 = 0;
			}
			
			// he behaves differently after every other time he pauses
			if (o->timer2)
			{
				if (++o->timer > 75)
				{
					o->frame = 0;
					o->state = STATE_PAUSE;
				}
			}
			else
			{
				if (++o->timer > 24)
					o->state = STATE_JUMP_FIRE;
			}
		}
		break;
		
		// jump and fire missiles
		case STATE_JUMP_FIRE:
		{
			o->state++;
			o->timer = 0;
			o->frame = 3;
			o->yinertia = -0x5ff;
		}
		case STATE_JUMP_FIRE+1:
		{
			FACEPLAYER;
			
			// fire missiles
			if (++o->timer < 30)
			{
				if ((o->timer % 6) == 1)
				{
					sound(SND_EM_FIRE);
					
					Object *shot = SpawnObjectAtActionPoint(o, OBJ_BALROG_MISSILE);
					shot->dir = o->dir;
					shot->xinertia = 0x100;
				}
			}
			
			// landed?
			if (o->blockd && o->yinertia >= 0)
			{
				o->frame = 2;
				o->state = STATE_PAUSE;
				quake(30);
			}
		}
		break;
		
		// stop for a moment
		case STATE_PAUSE:
		{
			o->xinertia *= 4;
			o->xinertia /= 5;
			if (o->xinertia != 0) break;
			
			o->state = 0;
		}
		break;
		
		case STATE_CAUGHT_PLAYER:	// caught player
		{
			if (balrog_toss_player_away(o))
				o->state = 0;
		}
		break;
	}
	
	o->yinertia += 0x20;
	LIMITX(0x300);
	LIMITY(0x5ff);
}

void ondeath_balrog_boss_missiles(Object *o)
{
	o->xinertia = 0;
}

static void walking_animation(Object *o)
{
	if (++o->animtimer > 3)
	{
		o->animtimer = 0;
		o->frame++;
		
		if (o->frame == 12)
		{
			sound(SND_THUD);
		}
		else if (o->frame > 12)
		{
			o->frame = 9;
		}
	}
}


/*
void c------------------------------() {}
*/

void ai_balrog_missile(Object *o)
{
	if ((o->dir == RIGHT && o->blockr) || \
		(o->dir == LEFT && o->blockl))
	{
		SmokeClouds(o, 3, 0, 0);
		effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
		sound(SND_MISSILE_HIT);
		
		o->Delete();
		return;
	}
	
	if (o->state == 0)
	{
		// recoil in oppisite direction
		o->xinertia = random(-2, -1) * CSFI;
		if (o->dir == LEFT) o->xinertia = -o->xinertia;
		
		o->yinertia = random(-2, 0) * CSFI;
		
		o->state = 1;
	}
	
	XACCEL(0x20);
	
	if ((++o->timer2 % 4) == 1)
	{
		effect(o->CenterX() - o->xinertia, o->CenterY(), EFFECT_SMOKETRAIL_SLOW);
	}
	
	// heat-seeking at start, then level out straight
	if (o->timer2 < 50)
	{
		if (o->y < player->y)
			o->yinertia += 0x20;
		else
			o->yinertia -= 0x20;
	}
	else
	{
		o->yinertia = 0;
	}
	
	// flash
	o->frame ^= 1;
	
	if (o->xinertia < -0x400)
		o->xinertia = -0x600;
	
	if (o->xinertia > 0x400)
		o->xinertia = 0x600;
}






















