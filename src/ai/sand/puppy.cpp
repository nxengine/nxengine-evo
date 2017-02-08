#include "puppy.h"
#include "../stdai.h"
#include "../ai.h"
#include "../../caret.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_PUPPY_WAG, ai_puppy_wag);
	ONTICK(OBJ_PUPPY_BARK, ai_puppy_bark);
	ONTICK(OBJ_PUPPY_SLEEP, ai_zzzz_spawner);
	ONTICK(OBJ_PUPPY_RUN, ai_puppy_run);
	
	AFTERMOVE(OBJ_PUPPY_CARRY, aftermove_puppy_carry);
}

/*
void c------------------------------() {}
*/

// these seem to be used for the the ones in jenka's house
// that you have already gotten.
void ai_puppy_wag(Object *o)
{
	// code shared with talking item-giving puppy from Plantation--
	// that one doesn't face you.
	if (o->type != OBJ_PUPPY_ITEMS)
	{
		FACEPLAYER;
	}
	
	// needed so you can talk to them immediately after giving them to jenka
	o->flags |= FLAG_SCRIPTONACTIVATE;
	
	// only wag when player is near
	if (pdistlx(56 * CSFI))
	{
		if (++o->animtimer >= 4)
		{
			o->animtimer = 0;
			o->animframe ^= 1;
		}
		
		o->frame = o->animframe;
	}
	else
	{
		o->animframe = 1;
		o->animtimer = 0;
		o->frame = 0;
	}
	
	randblink(o, 2, 4);
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}


void ai_puppy_bark(Object *o)
{
#define BARK	5
#define NOBARK	3

	if (o->state < 100)
	{
		FACEPLAYER;
	}
	
	switch(o->state)
	{
		case 0:
		case 100:
			o->state++;
			o->animframe = 0;
			o->timer2 = 0;
		case 1:
		case 101:
			// bark when player is near
			// note: this is also supposed to run at jenka's house when balrog appears
			// but it's ok:
			// the player is always near enough because of the way the cutscene is set up
			if ((abs(o->x - player->x) < (64 * CSFI)) && ((abs(o->y - player->y) < (16 * CSFI))))
			{
				if (++o->animtimer > 6)
				{
					o->animtimer = 0;
					
					if (o->frame==NOBARK)
					{
						o->frame = BARK;
						sound(SND_PUPPY_BARK);
					}
					else
					{
						o->frame = NOBARK;
						
						if (random(0, 8)==0 || ++o->timer2 > 5)
						{	// stop barking for a sec
							o->state++;
							o->timer = o->timer2 = 0;
							o->frame = 0;
						}
					}
				}
			}
			else
			{
				o->frame = 0;
				o->animtimer = 9999;		// begin barking as SOON as player gets near
			}
		break;
		
		case 2:
		case 102:
			o->frame = 0;
			if (++o->timer > 8)
			{	// start barking again
				o->state--;
				o->animtimer = 0;
				o->timer2 = 0;
			}
		break;
		
		// do not bark at all--set during jenka's 1st cutscene with balrog
		case 20:
		case 120:
			o->frame = 0;
		break;
	}
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}


void ai_zzzz_spawner(Object *o)
{
	if (!o->timer)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_ZZZZ);
		o->timer = 99;
	}
	else o->timer--;
}

void ai_puppy_run(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			o->flags |= FLAG_SCRIPTONACTIVATE;		// for some reason this isn't set on puppy in map
			o->state = 1;
		case 1:
			FACEPLAYER;
			o->frame = 0;		// necessary for randblink
			
			if (pdistly2((32 * CSFI), (16 * CSFI)))
			{
				if (pdistlx(32 * CSFI))
				{	// run away!!!
					FACEAWAYPLAYER;
					o->state = 10;
				}
				else if (pdistlx(96 * CSFI))
				{
					// wag tail
					if (++o->animtimer >= 4)
					{
						o->animtimer = 0;
						o->animframe ^= 1;
					}
					
					o->frame = o->animframe;
				}
			}
			
			randblink(o, 2, 4);
		break;
		
		case 10:
			o->state = 11;
			o->frame = 4;
			o->animtimer = 0;
		case 11:		// running
			if (o->blockd)
			{
				if (++o->animtimer > 2)
				{
					o->animtimer = 0;
					o->frame = (o->frame==4) ? 3:4;
				}
			}
			
			// "bounce" off walls
			if (o->dir==RIGHT)
			{
				if (o->blockr)
				{
					o->xinertia = -(o->xinertia >> 1);
					o->dir = LEFT;
				}
			}
			else
			{
				if (o->blockl)
				{
					o->xinertia = -(o->xinertia >> 1);
					o->dir = RIGHT;
				}
			}
			
			o->xinertia += (o->dir==RIGHT) ? 0x40 : -0x40;
			
			if (o->xinertia > 0x5ff) o->xinertia = 0x400;
			if (o->xinertia < -0x5ff) o->xinertia = -0x400;
		break;
	}
}

/*
void c------------------------------() {}
*/


void aftermove_puppy_carry(Object *o)
{
	StickToPlayer(o, -1, -13, -23);
	
	o->frame = 0;
	randblink(o, 2, 4);
}







