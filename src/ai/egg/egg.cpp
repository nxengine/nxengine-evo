#include "egg.h"
#include "../stdai.h"
#include "../ai.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../weed/weed.h"

#include "../../game.h"
#include "../../player.h"
#include "../../graphics/graphics.h"
#include "../../graphics/tileset.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CRITTER_HOPPING_GREEN, ai_critter);
	
	ONTICK(OBJ_BASIL, ai_basil);
	ONTICK(OBJ_BEHEMOTH, ai_behemoth);
	
	ONTICK(OBJ_BEETLE_GREEN, ai_beetle_horiz);
	ONTICK(OBJ_BEETLE_FREEFLY, ai_beetle_freefly);
	
	ONTICK(OBJ_GIANT_BEETLE, ai_giant_beetle);
	ONTICK(OBJ_GIANT_BEETLE_SHOT, ai_generic_angled_shot);
	
	ONTICK(OBJ_FORCEFIELD, ai_forcefield);
	ONTICK(OBJ_EGG_ELEVATOR, ai_egg_elevator);
}

/*
void c------------------------------() {}
*/

void ai_basil(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->x = player->x;
			o->state = (o->dir == LEFT) ? 1 : 2;
		break;
		
		case 1:
		{
			o->xinertia -= 0x40;
			
			if (o->blockl || ((player->x - o->x) >= (192 * CSFI)))
			{
				if (o->blockl) o->xinertia = 0;
				o->state = 2;
			}
		}
		break;
		
		case 2:
		{
			o->xinertia += 0x40;
			
			if (o->blockr || ((o->x - player->x) >= (192 * CSFI)))
			{
				if (o->blockr) o->xinertia = 0;
				o->state = 1;
			}
		}
		break;
	}
	
	if (o->xinertia > 0x80) o->dir = RIGHT;
	if (o->xinertia < -0x80) o->dir = LEFT;
	ANIMATE(1, 0, 2);
	LIMITX(0x5ff);
}


void ai_behemoth(Object *o)
{
	if (o->blockl) o->dir = RIGHT;
	if (o->blockr) o->dir = LEFT;
	
	switch(o->state)
	{
		case 0:
		{
			o->nxflags |= NXFLAG_THUD_ON_RIDING;
			o->damage = 1;
		}
		case 1:		// walking happily
		{
			XMOVE(0x100);
			ANIMATE(8, 0, 3);
			
			if (o->shaketime)
			{
				o->frame = 4;
				o->state = 2;
				o->timer = 0;
			}
		}
		break;
		
		case 2:		// grimace/hit
		{
			o->xinertia *= 7;
			o->xinertia /= 8;
			
			if (++o->timer > 40)
			{
				// player still picking on him?
				if (o->shaketime)
				{
					o->state = 3;
					o->frame = 6;
					o->animtimer = 0;
					o->timer = 0;
					
					o->damage = 5;
				}
				else
				{
					o->state = 1;
					o->animtimer = 0;
				}
			}
		}
		break;
		
		case 3:		// charge!
		{
			XMOVE(0x400);
			ANIMATE(5, 5, 6);
			
			if (++o->timer > 200)
				o->state = 0;
		}
		break;
	}
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}



/*
void c------------------------------() {}
*/

void ai_beetle_horiz(Object *o)
{
	enum { FLYING = 0, ON_WALL = 1 };
	
	if (o->state == FLYING)
	{
		if (++o->animtimer == 2)
		{
			o->animtimer = 0;
			o->animframe ^= 1;
		}
		
		o->frame = (o->animframe + 1);
		
		if (o->dir == RIGHT)
		{
			o->xinertia += 0x50;
			if (o->xinertia > 0x32c) o->xinertia = 0x32c;
			
			if (o->blockr)
			{
				o->dir = LEFT;
				o->state = ON_WALL;
				o->frame = 0;
				o->xinertia = 0;
			}
		}
		else
		{
			o->xinertia -= 0x50;
			if (o->xinertia < -0x32c) o->xinertia = -0x32c;
			
			if (o->blockl)
			{
				o->dir = RIGHT;
				o->state = ON_WALL;
				o->frame = 0;
				o->xinertia = 0;
			}
		}
	}
	else
	{	// waiting on wall
		if (abs(o->y - player->y) < (12 * CSFI))
		{
			if ((o->dir == RIGHT && (player->x > o->x)) || \
				(o->dir == LEFT && (player->x < o->x)))
			{
				o->animframe = 0;
				o->state = FLYING;
			}
		}
	}
}

void ai_beetle_freefly(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->invisible = true;
			o->flags &= ~(FLAG_SHOOTABLE | FLAG_SOLID_MUSHY);
			o->damage = 0;
			
			o->nxflags |= NXFLAG_SLOW_WHEN_HURT;
			o->state = 1;
		}
		case 1:
		{
			// has player made it to our position yet?
			// (we are invisible right now, our position on the map
			//  is a marker for when we should fly onto the screen)
			if (pdistlx(TILE_W * CSFI))
			{
				o->flags |= (FLAG_SHOOTABLE | FLAG_SOLID_MUSHY);
				o->damage = 2;
				o->invisible = false;
				
				o->ymark = o->y;
				o->yinertia = -0x100;
				
				o->state = 2;
				
				// come on screen
				if (o->dir == LEFT)
				{
					o->x = player->x + (256 * CSFI);
					o->xinertia = -0x2FF;
				}
				else
				{
					o->x = player->x - (256 * CSFI);
					o->xinertia = 0x2FF;
				}
			}
		}
		break;
		
		case 2:
		{
			FACEPLAYER;
			ANIMATE(1, 0, 1);
			XACCEL(0x10);
			
			// sinusoidal flight
			if (o->y > o->ymark)
				o->yinertia -= 0x10;
			else
				o->yinertia += 0x10;
			
			LIMITX(0x2FF);
			LIMITY(0x100);
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_giant_beetle(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->invisible = true;
			o->flags &= ~FLAG_SHOOTABLE;
			o->damage = 0;
			
			o->xmark = o->x;
			o->ymark = o->y;
			o->timer2 = o->dir;
			
			o->nxflags |= NXFLAG_SLOW_WHEN_HURT;
			o->state = 1;
		case 1:
		{
			if (pdistlx(TILE_W * CSFI))
			{
				o->flags |= FLAG_SHOOTABLE;
				o->damage = 6;
				o->invisible = false;
				
				o->yinertia = -0x100;
				
				o->state = 2;
				o->timer = 0;
				
				if (o->dir == LEFT)
				{
					o->x = player->x + (256 * CSFI);
					o->xinertia = -0x2FF;
				}
				else
				{
					o->x = player->x - (256 * CSFI);
					o->xinertia = 0x2FF;
				}
			}
		}
		break;
		
		case 2:
		{
			FACEPLAYER;
			XACCEL(0x10);
			
			if (o->blockl) o->xinertia = 0x200;
			if (o->blockr) o->xinertia = -0x200;
			
			// sinusoidal flight
			if (o->y > o->ymark)
				o->yinertia -= 0x08;
			else
				o->yinertia += 0x08;
			
			LIMITX(0x2FF);
			LIMITY(0x100);
			
			// if player runs too far away from us reset to our
			// waiting state and original position.
			if (!pdistlx((SCREEN_WIDTH + 80) * CSFI))
			{
				o->state = 0;
				o->xinertia = 0;
				o->yinertia = 0;
				
				o->x = o->xmark;
				//o->y = o->ymark;
				o->dir = o->timer2;
			}
			
			// animate wings. must do it this way instead of
			// using ANIMATE(), so that the frame>1 is only checked
			// when animtimer overflows, else flashing-before-fire
			// will get messed up.
			if (++o->animtimer > 1)
			{
				o->animtimer = 0;
				if (++o->frame > 1)
					o->frame = 0;
			}
			
			if (++o->timer >= 150)
			{
				// flashing before fire
				if (o->frame == 1)
					o->frame = 2;
				
				// after a bit of flashing, fire
				if (o->timer >= 158)
				{
					if (pdistlx(0x14000))
					{
						sound(SND_EM_FIRE);
						EmFireAngledShot(o, OBJ_GIANT_BEETLE_SHOT, 2, 0x400);
					}
					
					o->timer = 0;
				}
			}
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

// lift in Egg Observation Room
void ai_egg_elevator(Object *o)
{
	// assume not moving
	o->yinertia = 0;
	
	switch(o->state)
	{
		case 0:
		{
			// set wrong in npc.tbl, at least for our engine
			o->flags &= ~FLAG_SOLID_MUSHY;
			o->flags |= FLAG_SOLID_BRICK;
			
			o->state = 1;
		}	// fall thru
		case 1:
		case 3:
		case 5:
		case 7:
		{
			if (++o->timer > 150)
			{
				o->timer = 0;
				o->state++;
			}
		}
		break;
		
		case 2:
		case 4:
		case 6:
		case 8:
		{
			if (++o->timer > (TILE_H * 4))
			{
				o->timer = 0;
				
				if (++o->state > 8)
					o->state = 1;
			}
			else
			{
				o->yinertia = \
					(o->state > 5) ? 0x200 : -0x200;
				
				ANIMATE(1, 0, 1);
			}
		}
		break;
	}
	
}


void ai_forcefield(Object *o)
{
	// need to reinforce these or you can walk through the
	// spawned forcefield that blocks exit from Igor battle
	o->flags |= (FLAG_SOLID_BRICK | FLAG_INVULNERABLE);
	
	ai_animate1(o);
}



















