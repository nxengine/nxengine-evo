#include "bubbler.h"
#include "../ai.h"
#include "weapons.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../game.h"
#include "../../player.h"
#include "../../input.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BUBBLER12_SHOT, ai_bubbler_l12);
	ONTICK(OBJ_BUBBLER3_SHOT, ai_bubbler_l3);
	ONTICK(OBJ_BUBBLER_SHARP, ai_bubbler_sharp);
}

/*
void c------------------------------() {}
*/

void ai_bubbler_l12(Object *o)
{
int decel;

	if (o->shot.level == 1 && o->state == 0)
	{
		o->nxflags |= NXFLAG_FOLLOW_SLOPE;
		o->state = 1;
		
		int variance = random(-0x100, 0x100);
		switch(o->shot.dir)
		{
			case LEFT:
			case RIGHT:	o->yinertia = variance; break;
			
			case UP:
			case DOWN:  o->xinertia = variance; break;
		}
	}
	
	if (damage_enemies(o))
	{
		o->Delete();
		return;
	}
	
	if (IsBlockedInShotDir(o))
	{
		shot_dissipate(o, EFFECT_FISHY);
		return;
	}
	
	if (--o->shot.ttl < 0)
	{
		effect(o->x, o->y, EFFECT_BUBBLE_BURST);
		o->Delete();
		return;
	}
	
	if (o->shot.level == 0)
		decel = 0x2A;
	else
		decel = 0x10;
	
	switch(o->shot.dir)
	{
		case LEFT:  o->xinertia += decel; break;
		case RIGHT: o->xinertia -= decel; break;
		case UP:	o->yinertia += decel; break;
		case DOWN:	o->yinertia -= decel; break;
	}
	
	if (o->frame < 3)
		ANIMATE_FWD(3);
}


void ai_bubbler_l3(Object *o)
{
	if (damage_enemies(o))
	{
		o->Delete();
		return;
	}
	
	if (--o->shot.ttl < 0 || !inputs[FIREKEY])
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		sound(SND_BUBBLER_LAUNCH);
		
		Object *shot = CreateObject(o->x, o->y, OBJ_BUBBLER_SHARP);
		shot->dir = player->dir;
		shot->shot.dir = (player->look) ? player->look : player->dir;
		shot->shot.damage = 3;
		shot->shot.ttl = 32;
		
		o->Delete();
		return;
	}
	
	if (o->state == 0)
	{
		int bigspeed   = random(0x200, 0x400);
		int smallspeed = (random(-4, 4) * CSFI) / 2;
		
		switch(o->shot.dir)
		{
			case LEFT:
				o->xinertia = -bigspeed;
				o->yinertia = smallspeed;
			break;
			
			case RIGHT:
				o->xinertia = bigspeed;
				o->yinertia = smallspeed;
			break;
			
			case UP:
				o->xinertia = smallspeed;
				o->yinertia = -bigspeed;
			break;
			
			case DOWN:
				o->xinertia = smallspeed;
				o->yinertia = bigspeed;
			break;
		}
		
		o->state = 1;
	}
	
	// track player
	if (o->CenterX() > player->CenterX()) o->xinertia -= 0x20;
	if (o->CenterX() < player->CenterX()) o->xinertia += 0x20;
	if (o->CenterY() > player->CenterY()) o->yinertia -= 0x20;
	if (o->CenterY() < player->CenterY()) o->yinertia += 0x20;
	
	// bouncy
	if (o->xinertia < 0 && o->blockl)
		o->xinertia = 0x400;
	
	if (o->xinertia > 0 && o->blockr)
		o->xinertia = -0x400;
	
	if (o->yinertia < 0 && o->blocku)
		o->yinertia = 0x400;
	
	if (o->yinertia > 0 && o->blockd)
		o->yinertia = -0x400;
	
	// animate
	if (o->frame < 3)
		ANIMATE_FWD(3);
}


void ai_bubbler_sharp(Object *o)
{
	if (run_shot(o, true))
		return;
	
	if (o->state == 0)
	{
		o->state = 1;
		
		int speed = (random(10, 16) * CSFI) / 2;
		switch(o->shot.dir)
		{
			case LEFT:  o->xinertia = -speed; break;
			case RIGHT: o->xinertia = speed; break;
			case UP:	o->yinertia = -speed; break;
			case DOWN:	o->yinertia = speed; break;
		}
		
		o->x -= o->xinertia;
		o->y -= o->yinertia;
	}
	else
	{
		ANIMATE(1, 0, 1);
	}
}


















