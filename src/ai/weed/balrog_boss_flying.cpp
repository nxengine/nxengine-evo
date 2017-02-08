#include "balrog_boss_flying.h"
#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"

#include "../../game.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../player.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#define STEPS_TO_PLAYER			100

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BALROG_BOSS_FLYING, ai_balrog_boss_flying);
	ONDEATH(OBJ_BALROG_BOSS_FLYING, ondeath_balrog_boss_flying);
	
	ONTICK(OBJ_BALROG_SHOT_BOUNCE, ai_balrog_shot_bounce);
}

/*
void c------------------------------() {}
*/

void ai_balrog_boss_flying(Object *o)
{
	enum
	{
		INIT = 0,
		WAIT_BEGIN,
		
		SHOOT_PLAYER,
		JUMP_BEGIN,
		JUMP_UP,
		FLYING,
		JUMP_END,
		LANDED
	};
	
	switch(o->state)
	{
		case 0:
			o->state = WAIT_BEGIN;
			o->timer = 0;
		case WAIT_BEGIN:	// wait at start of battle
		{
			FACEPLAYER;
			if (++o->timer > 12)
			{
				o->state = SHOOT_PLAYER;
				o->timer = 0;
				o->timer2 = 0;
				o->frame = 1;
			}
		}
		break;
		
		case SHOOT_PLAYER:
		{
			FACEPLAYER;
			if (++o->timer > 16)
			{
				o->timer = 0;
				o->frame = 1;
				
				EmFireAngledShot(o, OBJ_IGOR_SHOT, 16, 0x200);
				sound(SND_EM_FIRE);
				
				if (++o->timer2 > 3)	// number of shots to fire
				{
					o->state = JUMP_BEGIN;
					o->timer = 0;
				}
			}
		}
		break;
		
		case JUMP_BEGIN:	// begin jump
		{
			FACEPLAYER;
			if (++o->timer > 3)
			{
				o->state = JUMP_UP;
				o->timer = 0;
				o->xinertia = (player->x - o->x) / STEPS_TO_PLAYER;
				o->yinertia = -0x600;
				o->frame = 3;
			}
		}
		break;
		
		case JUMP_UP:		// jumping up
		{
			if (o->yinertia > 0x200)
			{
				if (o->hp <= 60)
				{	// skip flying if low health
					o->state = JUMP_END;
				}
				else
				{
					o->state = FLYING;
					o->timer = 0;
					o->ymark = o->y;
					
					o->frame = 13;
					o->animtimer = 0;
				}
			}
		}
		break;
		
		case FLYING:
		{
			if (++o->animtimer > 1)
			{
				o->animtimer = 0;
				o->frame = (o->frame == 13) ? 14 : 13;
				// wings just went down
				if (o->frame == 13)
					sound(SND_EXPLOSION2);
			}
			
			if (++o->timer >= STEPS_TO_PLAYER)
			{
				o->state = JUMP_END;
				o->frame = 3;
			}
			
			o->yinertia += (o->y >= o->ymark) ? -0x40 : 0x40;
			LIMITY(0x200);
		}
		break;
		
		case JUMP_END:		// coming down from jump
		{
			if ((o->y + (16 * CSFI)) < player->y)
			{
				o->damage = 10;
			}
			else
			{
				o->damage = 0;
			}
			
			if (o->blockd)
			{
				o->xinertia = 0;
				o->damage = 0;
				
				sound(SND_FUNNY_EXPLODE);
				quake(30);
				
				SmokeSide(o, 6, DOWN);
				
				for(int i=0;i<8;i++)
				{
					int x = random(o->Left(), o->Right());
					int y = o->Bottom() - (8 * CSFI);
					Object *shot = CreateObject(x, y, OBJ_BALROG_SHOT_BOUNCE);
					
					shot->xinertia = random(-0x400, 0x400);
					shot->yinertia = random(-0x400, 0);
				}
				
				o->state = LANDED;
				o->timer = 0;
				o->frame = 2;
			}
		}
		break;
		
		case LANDED:
		{
			o->frame = 2;
			if (++o->timer > 3)
			{
				o->state = SHOOT_PLAYER;
				o->timer = 0;
			}
		}
		break;
	}
	
	if (o->state != FLYING)
		o->yinertia += 0x33;
	
	LIMITY(0x5FF);
}

void ondeath_balrog_boss_flying(Object *o)
{
	o->xinertia = 0;
	o->flags &= ~FLAG_SHOOTABLE;
	
	KillObjectsOfType(OBJ_BALROG_SHOT_BOUNCE);
	KillObjectsOfType(OBJ_IGOR_SHOT);
}


void ai_balrog_shot_bounce(Object *o)
{
	ANIMATE(2, 0, 1);
	
	if (o->blockd)
		o->yinertia = -0x400;
	
	o->yinertia += 42;
	
	if ((o->blockl && o->xinertia < 0) || \
		(o->blockr && o->xinertia > 0) || \
		++o->timer > 250)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
}









