#include "oside.h"
#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../sand/sand.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../map.h"
#include "../../game.h"
#include "../../player.h"

#include "../../graphics/graphics.h"
using namespace Graphics;
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_SKY_DRAGON, ai_sky_dragon);
	ONTICK(OBJ_SANDCROC_OSIDE, ai_sandcroc);
	
	ONTICK(OBJ_NIGHT_SPIRIT, ai_night_spirit);
	ONTICK(OBJ_NIGHT_SPIRIT_SHOT, ai_night_spirit_shot);
	
	ONTICK(OBJ_HOPPY, ai_hoppy);
	
	ONTICK(OBJ_PIXEL_CAT, ai_pixel_cat);
	ONTICK(OBJ_LITTLE_FAMILY, ai_little_family);
}

/*
void c------------------------------() {}
*/

#define NS_WAIT					1
#define NS_SEEK_PLAYER			2
#define NS_PREPARE_FIRE			3
#define NS_FIRING				4
#define NS_RETURN_TO_SET_POINT	5
#define NS_GUARD_SET_POINT		6
void ai_night_spirit(Object *o)
{
	//debug("ns state: %d", o->state);
	switch(o->state)
	{
		case 0:
		{
			o->state = NS_WAIT;
			o->nxflags |= NXFLAG_SLOW_WHEN_HURT;
			o->invisible = true;
			o->ymark = o->y + (12 * CSFI);
		}
		case NS_WAIT:
		{
			if (pdistly((TILE_H / 2) * CSFI))
			{
				static const int distance = (SCREEN_HEIGHT * CSFI);
				o->y += (o->dir == RIGHT) ? distance : -distance;
				
				o->state = NS_SEEK_PLAYER;
				o->timer = 0;
				o->invisible = false;
				
				o->yinertia = 0;
				o->flags |= FLAG_SHOOTABLE;
			}
		}
		break;
		
		case NS_SEEK_PLAYER:
		{
			ANIMATE(2, 0, 2);
			
			if (++o->timer > 200)
			{
				o->state = NS_PREPARE_FIRE;
				o->timer = 0;
				o->frame += 3;
			}
		}
		break;
		
		case NS_PREPARE_FIRE:
		{
			ANIMATE(2, 3, 5);
			if (++o->timer > 50)
			{
				o->state = NS_FIRING;
				o->timer = 0;
				o->frame += 3;
			}
		}
		break;
		
		case NS_FIRING:
		{
			ANIMATE(2, 6, 8);
			
			if ((++o->timer % 5) == 1)
			{
				Object *shot = SpawnObjectAtActionPoint(o, OBJ_NIGHT_SPIRIT_SHOT);
				shot->xinertia = random(0x100, 0x600);
				shot->yinertia = random(-0x200, 0x200);
				
				sound(SND_BUBBLE);
			}
			
			if (o->timer > 50)
			{
				o->state = NS_SEEK_PLAYER;
				o->timer = 0;
				o->frame -= 6;
			}
		}
		break;
		
		case NS_RETURN_TO_SET_POINT:
		{
			ANIMATE(2, 3, 5);
			
			// lie in wait at original set point
			o->yinertia += (o->y > o->ymark) ? -0x40 : 0x40;
			LIMITY(0x400);
			
			if (abs(o->y - o->ymark) < (SCREEN_HEIGHT/2) * CSFI)
			{
				o->state = NS_GUARD_SET_POINT;
			}
		}
		break;
		
		case NS_GUARD_SET_POINT:
		{
			ANIMATE(2, 3, 5);
			
			// lie in wait at original set point
			o->yinertia += (o->y > o->ymark) ? -0x40 : 0x40;
			LIMITY(0x400);
			
			// and if player appears again...
			if (pdistly(SCREEN_HEIGHT * CSFI))
			{	// ..jump out and fire immediately
				o->state = NS_PREPARE_FIRE;
				o->timer = 0;
			}
		}
		break;
	}
	
	if (o->state >= NS_SEEK_PLAYER && o->state < NS_GUARD_SET_POINT)
	{
		// sinusoidal player seek
		o->yinertia += (o->y < player->y) ? 0x19 : -0x19;
		
		// rarely seen, but they do bounce off walls
		if (o->blocku) o->yinertia = 0x200;
		if (o->blockd) o->yinertia = -0x200;
		
		//debugHline(o->ymark - (SCREEN_HEIGHT  * CSFI), 0, 255, 0);
		
		// avoid leaving designated area
		if (abs(o->y - o->ymark) > SCREEN_HEIGHT * CSFI)
		{
			if (o->state != NS_FIRING)
			{
				o->state = NS_RETURN_TO_SET_POINT;
			}
		}
	}
	
	LIMITY(0x400);
}

void ai_night_spirit_shot(Object *o)
{
	ANIMATE(2, 0, 2);
	o->xinertia -= 0x19;
	
	if (o->xinertia >= 0)
	{
		o->flags |= FLAG_IGNORE_SOLID;
	}
	else
	{
		o->flags &= ~FLAG_IGNORE_SOLID;
		if (o->blockl)
		{
			SmokeClouds(o, 4, 3, 3);
			sound(SND_SHOT_HIT);
			o->Delete();
		}
	}
}

/*
void c------------------------------() {}
*/

void ai_hoppy(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
		}
		case 1:		// wait for player...
		{
			o->frame = 0;
			if (pdistly(0x10000))
			{
				o->state = 2;
				o->timer = 0;
				o->frame = 1;
			}
		}
		break;
		
		case 2:	// jump
		{
			o->timer++;
			
			if (o->timer == 4)
				o->frame = 2;
			
			if (o->timer > 12)
			{
				o->state = 3;
				o->frame = 3;
				
				sound(SND_HOPPY_JUMP);
				o->xinertia = 0x700;
			}
		}
		break;
		
		case 3:	// in air...
		{
			if (o->y < player->y)	   o->yinertia = 0xAA;
			else if (o->y > player->y) o->yinertia = -0xAA;
			
			if (o->blockl)
			{
				// don't stop if all of the blockl's are just touching slope
				bool stop = false;
				for(int i=0;i<sprites[o->sprite].block_l.count;i++)
				{
					uint32_t attr = o->GetAttributes(&sprites[o->sprite].block_l[i], 1, NULL);
					if (!(attr & TA_SLOPE))
					{
						stop = true;
						break;
					}
				}
				
				if (stop)
				{
					o->xinertia = 0;
					o->yinertia = 0;
					
					o->state = 4;
					o->frame = 2;
					o->timer = 0;
				}
			}
		}
		break;
		
		case 4:
		{
			o->timer++;
			if (o->timer == 2) o->frame = 1;
			if (o->timer == 6) o->frame = 0;
			
			if (o->timer > 16)
				o->state = 1;
		}
		break;
	}
	
	o->xinertia -= 0x2A;
	LIMITX(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_sky_dragon(Object *o)
{
	switch(o->state)
	{
		case 0:		// standing
		{
			ANIMATE(30, 0, 1);
		}
		break;
		
		case 10:	// player and kazuma gets on, dragon floats up
		{
			o->state = 11;
			o->frame = 2;
			o->animtimer = 0;
			
			o->xmark = o->x - (6 * CSFI);
			o->ymark = o->y - (16 * CSFI);
			
			o->yinertia = 0;
		}
		case 11:
		{
			ANIMATE(5, 2, 3);
			o->xinertia += (o->x < o->xmark) ? 0x08 : -0x08;
			o->yinertia += (o->y < o->ymark) ? 0x08 : -0x08;
		}
		break;
		
		case 20:	// fly away
		{
			ANIMATE(2, 2, 3);
			
			o->yinertia += (o->y < o->ymark) ? 0x08 : -0x08;
			o->xinertia += 0x20;
			LIMITX(0x600);
			
			/*player->x = o->x;
			player->y = o->y;
			run_phase_compensator();*/
		}
		break;
		
		case 30:	// spawn a Sue hanging from mouth
		{
		    Object *sue = CreateObject(o->x, o->y, OBJ_SUE);
		    sue->state = 42;
		    sue->dir = 0;
		    sue->frame = 9;
		    sue->sue.carried_by = o;
		}
		break;
	}
}

/*
void c------------------------------() {}
*/


void ai_pixel_cat(Object *o)
{
	#ifdef SHOW_DEBUG_KITTY
	if (o->state == 0)
	{
		o->y -= (32 * CSFI);
		o->state = 1;
	}
	#endif
}


void ai_little_family(Object *o)
{
	o->frame &= 1;
	
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->frame = 0;
			o->xinertia = 0;
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
		}
		case 1:
		{
			if (!random(0, 60))
			{
				o->state = random(0, 1) ? 2 : 10;
				o->timer = 0;
				o->frame = 1;
			}
		}
		break;
		
		case 2:
		{
			if (++o->timer > 8)
			{
				o->state = 1;
				o->frame = 0;
			}
		}
		break;
		
		case 10:
		{
			o->state = 11;
			o->frame = 0;
			o->animtimer = 0;
			o->dir = random(0, 1) ? LEFT : RIGHT;
			o->timer = random(16, 32);
		}
		case 11:
		{
			if ((o->blockl && o->dir == LEFT) || \
				(o->blockr && o->dir == RIGHT))
			{
				o->dir ^= 1;
			}
			
			XMOVE(0x100);
			ANIMATE(4, 0, 1);
			
			if (--o->timer < 0)
				o->state = 0;
		}
		break;
	}
	
	switch(o->id2)
	{
		case 210: o->frame += 2; break;		// red mom
		case 220: o->frame += 4; break;		// little son
	}
	
	o->yinertia += 0x20;
	LIMITY(0x5ff);
}




















