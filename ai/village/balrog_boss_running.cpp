#include "balrog_boss_running.h"
#include "../stdai.h"
#include "../ai.h"
#include "../balrog_common.h"
#include "../maze/balrog_boss_missiles.h"

#include "../../game.h"
#include "../../player.h"
#include "../../sound/sound.h"

#define STATE_CHARGE			10
#define STATE_JUMP				20
#define STATE_SLOW_DOWN			30
#define STATE_CAUGHT_PLAYER		40

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BALROG_BOSS_RUNNING, ai_balrog_boss_running);
	ONDEATH(OBJ_BALROG_BOSS_RUNNING, ondeath_balrog_boss_running);
}

/*
void c------------------------------() {}
*/

void ai_balrog_boss_running(Object *o)
{
	// try to catch player
	if (o->state == STATE_CHARGE+1 || o->state == STATE_JUMP)
	{
		if ((pdistlx(12 << CSF) && pdistly(8 << CSF)) && o->timer > 8)
		{
			hurtplayer(2);
			balrog_grab_player(o);
			o->state = STATE_CAUGHT_PLAYER;
		}
	}
	
	switch(o->state)
	{
		case 0:
		{
			FACEPLAYER;
			o->flags |= FLAG_SHOOTABLE;
			o->frame = 0;
			o->state = 1;
		}
		case 1:
		{
			if (++o->timer > 30)
			{
				o->state = STATE_CHARGE;
				o->timer2++;
			}
		}
		break;
		
		// running towards player
		case STATE_CHARGE:
		{
			o->state++;
			o->timer = 0;
			o->frame = 9;
			o->animtimer = 0;
		}
		case STATE_CHARGE+1:
		{
			XACCEL(0x10);
			walking_animation(o);
			
			if (++o->timer > 75 || \
				(o->dir == LEFT && o->blockl) ||
				(o->dir == RIGHT && o->blockr))
			{
				o->frame = 0;
				o->state = STATE_SLOW_DOWN;
				break;
			}
			
			// can jump every 3rd time, but if he catches the player
			// before he gets a chance to he does NOT jump on the next charge.
			if ((o->timer2 & 3) == 0)
			{
				if (o->timer > 25)
				{	// initiate jump
					o->frame = 3;
					o->yinertia = -0x400;
					o->state = STATE_JUMP;
				}
			}
		}
		break;
		
		// jumping
		case STATE_JUMP:
		{
			if (o->blockd && o->yinertia >= 0)
			{
				o->frame = 2;		// <-- Landed frame.
				quake(30);
				o->state = STATE_SLOW_DOWN;
			}
		}
		break;
		
		// slowing down after charging or jumping
		case STATE_SLOW_DOWN:
		{
			o->xinertia *= 4;
			o->xinertia /= 5;
			
			if (o->xinertia == 0)
				o->state = 0;
		}
		break;
		
		// caught player
		case STATE_CAUGHT_PLAYER:
		{
			if (balrog_toss_player_away(o))
				o->state = 0;
		}
		break;
	}
	
	o->yinertia += 0x20;
	LIMITX(0x400);
	LIMITY(0x5FF);
}

static void walking_animation(Object *o)
{
	if (++o->animtimer > 3)
	{
		o->animtimer = 0;
		
		if (++o->frame > 12)
			o->frame = 9;
		
		if (o->frame == 10 || o->frame == 12)
		{
			sound(SND_THUD);
		}
	}
}

/*
void c------------------------------() {}
*/

void ondeath_balrog_boss_running(Object *o)
{
	o->xinertia = 0;
	o->yinertia = 0;
}


