#include "ironhead.h"
#include "../stdai.h"
#include "../ai.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../tsc.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../map.h"
#include "../../graphics/graphics.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"



#define ARENA_TOP				2
#define ARENA_BOTTOM			13

#define IRONH_SPAWN_FISHIES		100
#define IRONH_SWIM				250
#define IRONH_DEFEATED			1000

INITFUNC(AIRoutines)
{
	ONDEATH(OBJ_IRONH, ondeath_ironhead);
	
	ONTICK(OBJ_IRONH_FISHY, ai_ironh_fishy);
	ONTICK(OBJ_IRONH_SHOT, ai_ironh_shot);
	
	ONTICK(OBJ_BRICK_SPAWNER, ai_brick_spawner);
	ONTICK(OBJ_IRONH_BRICK, ai_ironh_brick);
	
	ONTICK(OBJ_IKACHAN_SPAWNER, ai_ikachan_spawner);
	ONTICK(OBJ_IKACHAN, ai_ikachan);
	
	ONTICK(OBJ_MOTION_WALL, ai_motion_wall);
	
	objprop[OBJ_IRONH].hurt_sound = SND_ENEMY_HURT_COOL;
}

/*
void c------------------------------() {}
*/

void IronheadBoss::OnMapEntry()
{
	o = CreateObject(0, 0, OBJ_IRONH);
	o->damage = 10;
	o->hp = 400;
	o->state = IRONH_SPAWN_FISHIES;
	
	game.stageboss.object = o;
	this->hittimer = 0;
}

void IronheadBoss::OnMapExit()
{
	o = NULL;
	game.stageboss.object = NULL;
}

/*
void c------------------------------() {}
*/

static void ironh_smokecloud(Object *o)
{
Object *smoke;

	smoke = CreateObject(o->CenterX() + (random(-128, 128) * CSFI), \
						 o->CenterY() + (random(-64, 64) * CSFI),
						 OBJ_SMOKE_CLOUD);
	
	smoke->xinertia = random(-128, 128);
	smoke->yinertia = random(-128, 128);
}


void IronheadBoss::Run(void)
{
	if (!o) return;
	
	switch(o->state)
	{
		case IRONH_SPAWN_FISHIES:
		{
			o->timer = 0;
			o->state++;
		}
		case IRONH_SPAWN_FISHIES+1:		// wave of fishies comes in
		{
			if (++o->timer > 50)
			{
				o->timer = 0;
				o->state = IRONH_SWIM;
			}
			
			if ((o->timer & 3)==0)
			{
				CreateObject((random(15, 18) * TILE_W) * CSFI, \
						  	(random(ARENA_TOP, ARENA_BOTTOM) * TILE_H) * CSFI, \
						  	OBJ_IRONH_FISHY);
			}
		}
		break;
		
		case IRONH_SWIM:		// swimming attack
		{
			o->state++;
			
			if (o->dir==RIGHT)
			{	// coming up on player from left
				o->x = 0x0e000;
				o->y = player->y;
			}
			else
			{	// returning from right side of screen
				o->x = 0x5a000;
				o->y = (random(ARENA_TOP, ARENA_BOTTOM) * TILE_H) * CSFI;
			}
			
			o->xmark = o->x;
			o->ymark = o->y;
			
			o->yinertia = random(-0x200, 0x200);
			o->xinertia = random(-0x200, 0x200);
			
			o->flags |= FLAG_SHOOTABLE;
		}
		case IRONH_SWIM+1:
		{
			ANIMATE(2, 0, 7);
			
			if (o->dir==RIGHT)
			{
				o->xmark += 0x400;
			}
			else
			{
				o->xmark -= 0x200;
				o->ymark += (o->ymark < player->y) ? 0x200: -0x200;
			}
			
			//debugXline(o->xmark, 255,0,0);
			//debugYline(o->ymark, 0,255,0);
			o->xinertia += (o->x > o->xmark) ? -8 : 8;
			o->yinertia += (o->y > o->ymark) ? -8 : 8;
			
			LIMITY(0x200);
			
			if (o->dir==RIGHT)
			{
				if (o->x > 0x5a000)
				{
					o->dir = LEFT;
					o->state = IRONH_SPAWN_FISHIES;
				}
			}
			else
			{
				if (o->x < 0x12000)
				{
					o->dir = RIGHT;
					o->state = IRONH_SPAWN_FISHIES;
				}
			}
			
			if (o->dir==LEFT)
			{
				// fire bullets at player when retreating
				switch(++o->timer)
				{
					case 300:
					case 310:
					case 320:
					{
						Object *shot = SpawnObjectAtActionPoint(o, OBJ_IRONH_SHOT);
						shot->xinertia = (random(-3, 0) * CSFI);
						shot->yinertia = (random(-3, 3) * CSFI);
						sound(SND_EM_FIRE);
					}
					break;
				}
			}
		}
		break;
		
		case IRONH_DEFEATED:
		{
			sound(SND_EXPL_SMALL);
			o->state = IRONH_DEFEATED+1;
			o->flags &= ~FLAG_SHOOTABLE;
			o->frame = 8;
			o->damage = 0;
			o->xmark = o->x;
			o->ymark = o->y;
			o->xinertia = o->yinertia = 0;
			o->timer = 0;
			KillObjectsOfType(OBJ_IRONH_FISHY);
			KillObjectsOfType(OBJ_IRONH_BRICK);
			KillObjectsOfType(OBJ_BRICK_SPAWNER);
			game.quaketime = 20;
			
			for(int i=0;i<32;i++)
				ironh_smokecloud(o);
		}
		case IRONH_DEFEATED+1:			// retreat back to left...
		{
			o->xmark -= (1 * CSFI);
			
			o->x = o->xmark + (random(-1, 1) * CSFI);
			o->y = o->ymark + (random(-1, 1) * CSFI);
			
			o->timer++;
			if ((o->timer & 3)==0) ironh_smokecloud(o);
		}
		break;
	}
	
	// show pink "hit" frame when he's taking damage
	o->sprite = SPR_IRONH;
	if (o->shaketime)
	{
		this->hittimer++;
		if (this->hittimer & 2)
		{
			o->sprite = SPR_IRONH_HURT;
		}
	}
	else
	{
		this->hittimer = 0;
	}
}


void ondeath_ironhead(Object *o)
{
	game.tsc->StartScript(1000);
}

/*
void c------------------------------() {}
*/

void ai_ironh_fishy(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 10;
			o->animtimer = 0;
			o->yinertia = random(-0x200, 0x200);
			o->xinertia = 0x800;
		}
		case 10:			// harmless fishy
		{
			ANIMATE(2, 0, 1);
			if (o->xinertia < 0)
			{
				o->damage = 3;
				o->state = 20;
			}
		}
		break;
		
		case 20:			// puffer fish
		{
			ANIMATE(2, 2, 3);
			
			if (o->x < (48 * CSFI))
				o->Delete();
		}
		break;
	}
	
	if (o->blocku) o->yinertia = 0x200;
	if (o->blockd) o->yinertia = -0x200;
	o->xinertia -= 0x0c;
}

void ai_ironh_shot(Object *o)
{
	if (!o->state)
	{
		if (++o->timer > 20)
		{
			o->state = 1;
			o->xinertia = o->yinertia = 0;
			o->timer2 = 0;
		}
	}
	else
	{
		o->xinertia += 0x20;
	}
	
	ANIMATE(0, 0, 2);
	
	if (++o->timer2 > 100 && !o->onscreen)
	{
		o->Delete();
	}
	
	if ((o->timer2 & 3)==1) sound(SND_IRONH_SHOT_FLY);
}


void ai_brick_spawner(Object *o)
{
Object *brick;

	if (!o->state)
	{
		o->state = 1;
		o->timer = random(0, 200);
	}
	
	if (!o->timer)
	{	// time to spawn a block
		o->state = 0;
		brick = CreateObject(o->x, o->y + (random(-20, 20) * CSFI), OBJ_IRONH_BRICK);
		brick->dir = o->dir;
	}
	else o->timer--;
}

void ai_ironh_brick(Object *o)
{
	if (!o->state)
	{
		int r = random(0, 9);
		if (r == 9)
		{
			o->sprite = SPR_IRONH_BIGBRICK;
		}
		else
		{
			o->sprite = SPR_IRONH_BRICK;
			o->frame = r;
		}
		
		o->xinertia = random(0x100, 0x200);
		o->xinertia *= (o->dir == LEFT) ? -2 : 2;
		
		o->yinertia = random(-0x200, 0x200);
		o->state = 1;
	}
	
	// bounce off the walls
	if (o->yinertia < 0 && o->y <= (16 * CSFI))
	{
		effect(o->CenterX(), o->y, EFFECT_BONKPLUS);
		o->yinertia = -o->yinertia;
	}
	
	if (o->yinertia > 0 && (o->Bottom() >= (239 * CSFI)))
	{
		effect(o->CenterX(), o->Bottom(), EFFECT_BONKPLUS);
		o->yinertia = -o->yinertia;
	}
	
	
	if ((o->xinertia < 0 && (o->x < -0x2000)) ||\
		(o->x > (map.xsize * TILE_W) * CSFI))
	{
		o->Delete();
	}
}

/*
void c------------------------------() {}
*/

void ai_ikachan_spawner(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			// oops player got hurt--no ikachans for you!
			// the deletion of the object causes the flag matching it's id2 to be set,
			// which is how the scripts know not to give the alien medal.
			if (player->hurt_time != 0)
				o->Delete();
		}
		break;
		
		case 10:	// yay spawn ikachans!
		{
			o->timer++;
			if ((o->timer & 3) == 1)
			{
				CreateObject(o->x, o->y + ((random(0, 13) * TILE_H) * CSFI), OBJ_IKACHAN);
			}
		}
		break;
	}
}

void ai_ikachan(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->timer = random(3, 20);
		}
		case 1:		// he pushes ahead
		{
			if (--o->timer <= 0)
			{
				o->state = 2;
				o->timer = random(10, 50);
				o->frame = 1;
				o->xinertia = 0x600;
			}
		}
		break;
		
		case 2:		// after a short time his tentacles look less whooshed-back
		{
			if (--o->timer <= 0)
			{
				o->state = 3;
				o->timer = random(40, 50);
				o->frame = 2;
				o->yinertia = random(-0x100, 0x100);
			}
		}
		break;
		
		case 3:		// gliding
		{
			if (--o->timer <= 0)
			{
				o->state = 1;
				o->timer = 0;
				o->frame = 0;
			}
			
			o->xinertia -= 0x10;
		}
		break;
	}
	
	if (o->x > 720 * CSFI) o->Delete();
}

void ai_motion_wall(Object *o)		// the walls at the top and bottom of the arena
{
	o->x -= (6 * CSFI);
	
	if (o->x < (((9 * TILE_W) - 8) * CSFI))
		o->x += (SCREEN_WIDTH + 160 + 32) * CSFI;
}

