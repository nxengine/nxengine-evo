#include "labyrinth_m.h"

#include "../stdai.h"
#include "../ai.h"

#include "../sym/smoke.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../autogen/sprites.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_FIREWHIRR, ai_firewhirr);
	ONTICK(OBJ_FIREWHIRR_SHOT, ai_firewhirr_shot);
	
	ONTICK(OBJ_GAUDI_EGG, ai_gaudi_egg);
	
	ONTICK(OBJ_FUZZ_CORE, ai_fuzz_core);
	ONTICK(OBJ_FUZZ, ai_fuzz);
	AFTERMOVE(OBJ_FUZZ, aftermove_fuzz);
	
	ONTICK(OBJ_BUYOBUYO_BASE, ai_buyobuyo_base);
	ONTICK(OBJ_BUYOBUYO, ai_buyobuyo);
}

/*
void c------------------------------() {}
*/

void ai_firewhirr(Object *o)
{
	FACEPLAYER;
	
	switch(o->state)
	{
		case 0:
			o->state = 1;
			o->timer = random(0, 50);
			o->ymark = o->y;
			//o->yinertia = -0x200;
		case 1:
			
			if (!o->timer)
			{
				o->state = 10;
				o->yinertia = -0x200;
			}
			else o->timer--;
			
		case 10:
			o->frame ^= 1;
			
			o->yinertia += (o->y < o->ymark) ? 0x10 : -0x10;
			LIMITY(0x200);
			
			// inc time-to-fire while player near
			if (pdistly(80 * CSFI))
			{
				if (o->dir==LEFT && player->x < o->x && pdistlx(160 * CSFI)) o->timer2++;
				if (o->dir==RIGHT && player->x > o->x && pdistlx(160 * CSFI)) o->timer2++;
			}
			
			// if time to fire, spawn a shot
			if (o->timer2 > 20)
			{
				SpawnObjectAtActionPoint(o, OBJ_FIREWHIRR_SHOT);
				o->timer2 = -100 + random(0, 20);
				// tell Curly to acquire us as a target
				if (o->onscreen)
				{
					o->CurlyTargetHere();
				}
			}
		break;
	}
}

void ai_firewhirr_shot(Object *o)
{
	ANIMATE(1, 0, 2);
	o->x += (o->dir==LEFT) ? -0x200 : 0x200;
	
	if ((o->dir==LEFT && o->blockl) || (o->dir==RIGHT && o->blockr))
	{
		if (o->dir == RIGHT) o->x += o->Width();
		effect(o->x, o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
}

/*
void c------------------------------() {}
*/

void ai_gaudi_egg(Object *o)
{
	if (!o->state)
	{
		if (o->dir==LEFT)
		{	// on floor
			// align properly with ground
			o->y -= (4 * CSFI);
			o->x -= (4 * CSFI);
		}
		else
		{	// on ceiling
			// for the egg @ entrance point that is on a ceiling slope
			if (!o->blocku)
			{
				o->y -= (14 * CSFI);
			}
		}
		o->state = 1;
	}
	else if (o->state == 1)
	{
		if (o->hp < 90)
		{
			o->frame = 1;
			o->damage = 0;
			o->flags &= ~FLAG_SHOOTABLE;
			SmokeSide(o, 6, (o->dir==LEFT)?DOWN:UP);
			o->SpawnPowerups();
			NXE::Sound::SoundManager::getInstance()->playSfx(objprop[o->type].death_sound);
			o->state = 2;
		}
	}
}

/*
void c------------------------------() {}
*/

void ai_fuzz_core(Object *o)
{
	ANIMATE(2, 0, 1);
	
	switch(o->state)
	{
		case 0:
		{
			// spawn mini-fuzzes
			int angle = 120;
			for(int i=0;i<5;i++)
			{
				Object *f = CreateObject(o->CenterX(), o->CenterY(), OBJ_FUZZ);
				f->linkedobject = o;
				f->angle = angle;
				angle += (256 / 5);
			}
			
			o->timer = random(1, 50);
			o->state = 1;
		}
		case 1:		// de-syncs the Y positions when multiple cores are present at once
		{
			if (--o->timer <= 0)
			{
				o->state = 2;
				o->yinertia = 0x300;
				o->ymark = o->y;
			}
		}
		break;
		
		case 2:
		{
			FACEPLAYER;
			
			if (o->y > o->ymark) o->yinertia -= 0x10;
			if (o->y < o->ymark) o->yinertia += 0x10;
			LIMITY(0x355);
		}
		break;
	}
}

void ai_fuzz(Object *o)
{
	FACEPLAYER;
	
	switch(o->state)
	{
		case 0:
		{
			o->angle += 4;
			
			if (!o->linkedobject)
			{
				o->xinertia = random(-0x200, 0x200);
				o->yinertia = random(-0x200, 0x200);
				o->state = 1;
			}
		}
		break;
		
		// base destroyed, simple sinusoidal player-seek
		case 1:
		{
			o->xinertia += (o->x > player->x) ? -0x20 : 0x20;
			o->yinertia += (o->y > player->y) ? -0x20 : 0x20;
			
			LIMITX(0x800);
			LIMITY(0x200);
		}
		break;
	}
}

void aftermove_fuzz(Object *o)
{
	if (o->state == 0 && o->linkedobject)
	{
		vector_from_angle(o->angle, (20 * CSFI), &o->x, NULL);
		vector_from_angle(o->angle, (32 * CSFI), NULL, &o->y);
		
		o->x += o->linkedobject->CenterX() - (o->Width() / 2);
		o->y += o->linkedobject->CenterY() - (o->Height() / 2);
	}
}

/*
void c------------------------------() {}
*/

#define BUYOBUYO_BASE_HP		60

void ai_buyobuyo_base(Object *o)
{
/*if (o->onscreen) {
	debug("state: %d", o->state);
	debug("timer: %d", o->timer);
	debug("timer2: %d", o->timer2);
}*/
	
	if (o->state < 3 && o->hp < (1000 - BUYOBUYO_BASE_HP))
	{
		SmokeClouds(o, objprop[o->type].death_smoke_amt, 8, 8);
		effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
		o->SpawnPowerups();
		
		o->flags &= ~FLAG_SHOOTABLE;
		o->damage = 0;
		
		o->state = 10;
		o->frame = 2;
	}
	
	switch(o->state)
	{
		case 0:
		{
			// ceiling has different bounding box and action point
			if (o->dir == RIGHT)
				o->sprite = SPR_BUYOBUYO_BASE_CEILING;
			
			o->state = 1;
			o->timer = 10;
		}
		case 1:
		{
			if (pdistlx(0x14000))
			{
				if ((o->dir == LEFT && pdistly2(0x14000, 0x2000)) || \
					(o->dir == RIGHT && pdistly2(0x2000, 0x14000)))
				{
					if (--o->timer < 0)
					{
						o->state = 2;
						o->timer = 0;
						o->animtimer = 0;
					}
				}
			}
		}
		break;
		
		case 2:
		{
			ANIMATE(3, 0, 1);
			
			if (++o->timer > 10)
			{
				Object *buyo = SpawnObjectAtActionPoint(o, OBJ_BUYOBUYO);
				buyo->dir = o->dir;
				
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
				o->frame = 0;
				o->CurlyTargetHere();
				
				// cyclic: three firings then pause
				o->state = 1;
				if (++o->timer2 > 2)
				{
					o->timer = 100;
					o->timer2 = 0;
				}
				else
				{
					o->timer = 20;
				}
			}
		}
		break;
	}
}

void ai_buyobuyo(Object *o)
{
	bool deleteme = false;
	ANIMATE(6, 0, 1);
	
	switch(o->state)
	{
		case 0:
		{
			// shoot up down at player...
			o->yinertia = (o->dir == LEFT) ? -0x600 : 0x600;
			o->state = 1;
			o->timer = 0;
		}
		case 1:
		{
			o->timer++;		// inc fly time
			// reached height of player yet?
			if (pdistly(0x2000))
			{
				o->state = 2;
				ai_buyobuyo(o);
				return;
			}
		}
		break;
		
		case 2:
		{
			// this slight "minimum fly time" keeps the underwater ones from
			// smacking into the floor if the player is underwater with them
			if (++o->timer > 3)
			{
				FACEPLAYER;
				o->xmark = o->x;
				o->ymark = o->y;
				
				o->xinertia = random(0, 1) ? 0x200 : -0x200;
				o->yinertia = random(0, 1) ? 0x200 : -0x200;
				
				o->state = 3;
			}
		}
		break;
		
		case 3:
		{
			if (o->x > o->xmark) o->xinertia -= 0x20;
			if (o->x < o->xmark) o->xinertia += 0x20;
			if (o->y > o->ymark) o->yinertia -= 0x20;
			if (o->y < o->ymark) o->yinertia += 0x20;
			LIMITX(0x400);
			LIMITY(0x400);
			
			// move the point we are bobbling around
			o->xmark += (o->dir == LEFT) ? -(1 * CSFI) : (1 * CSFI);
			//debugVline(o->xmark, 0, 0xff, 0);
			
			if (++o->timer > 300)
				deleteme = true;
		}
		break;
	}
	
	if ((o->blockl && o->xinertia < 0) || \
		(o->blockr && o->xinertia > 0) || \
		(o->blocku && o->yinertia < 0) || \
		(o->blockd && o->yinertia > 0))
	{
		deleteme = true;
	}

	if (deleteme)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_STARPOOF);
		o->Delete();
		return;
	}
}
