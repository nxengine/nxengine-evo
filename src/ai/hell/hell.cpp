#include "hell.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../map.h"
#include "../../caret.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../player.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"


#define BUTE_HP			4
#define MESA_HP			64
#define DELEET_HP		32
#define STATUE_HP		100

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BUTE_FLYING, ai_bute_flying);
	ONTICK(OBJ_BUTE_DYING, ai_bute_dying);
	
	ONTICK(OBJ_BUTE_SPAWNER, ai_bute_spawner);
	ONTICK(OBJ_BUTE_FALLING, ai_bute_falling);
	
	ONTICK(OBJ_BUTE_SWORD, ai_bute_sword);
	ONTICK(OBJ_BUTE_ARCHER, ai_bute_archer);
	ONTICK(OBJ_BUTE_ARROW, ai_bute_arrow);
	
	ONTICK(OBJ_MESA, ai_mesa);
	ONTICK(OBJ_MESA_BLOCK, ai_mesa_block);
	ONTICK(OBJ_MESA_DYING, ai_bute_dying);
	
	ONTICK(OBJ_DELEET, ai_deleet);
	ONTICK(OBJ_ROLLING, ai_rolling);
	
	ONTICK(OBJ_STATUE, ai_statue);
	ONTICK(OBJ_STATUE_BASE, ai_statue_base);
	
	ONTICK(OBJ_PUPPY_GHOST, ai_puppy_ghost);
}

/*
void c------------------------------() {}
*/

void ai_bute_flying(Object *o)
{
	//AIDEBUG;
	if (run_bute_defeated(o, BUTE_HP))
		return;
	
	switch(o->state)
	{
		case 0:
		{
			o->invisible = true;
			o->state = 1;
		}
		case 1:
		{
			if (o->dir == LEFT)
			{
				if (player->x > (o->x - (288 * CSFI)) && \
					player->x < (o->x - (272 * CSFI)))
				{
					o->state = 10;
				}
			}
			else
			{
				if (player->x < (o->x + (288 * CSFI)) && \
					player->x > (o->x + (272 * CSFI)))
				{
					o->state = 10;
				}
			}
		}
		break;
		
		case 10:
		{
			o->state = 11;
			o->invisible = false;
			o->flags |= FLAG_SHOOTABLE;
			o->damage = 5;
		}
		case 11:
		{
			FACEPLAYER;
			ANIMATE(1, 0, 1);
			
			XACCEL(0x10);
			o->yinertia += (o->y > player->y) ? -0x10 : 0x10;
			
			LIMITX(0x5ff);
			LIMITY(0x5ff);
			
			if ((o->blockl && o->xinertia < 0) || \
				(o->blockr && o->xinertia > 0))
			{
				o->xinertia = -o->xinertia;
			}
			
			if ((o->blockd && o->yinertia > 0) || \
				(o->blocku && o->yinertia < 0))
			{
				o->yinertia = -o->yinertia;
			}
			
			
		}
		break;
	}
	
}


/*
void c------------------------------() {}
*/

// Butes that come down from ceiling
void ai_bute_spawner(Object *o)
{
static const int NUM_BUTES = 8;

	switch(o->state)
	{
		case 10:	// script trigger (dir set by script at same time)
		{
			o->state = 11;
			o->timer = 0;
		}
		case 11:
		{
			o->timer++;
			
			if ((o->timer % 50) == 1)
			{
				CreateObject(o->x, o->y, OBJ_BUTE_FALLING, 0, 0, o->dir);
				
				if (o->timer == ((NUM_BUTES - 1) * 50) + 1)
					o->state = 0;
			}
		}
		break;
	}
}

void ai_bute_falling(Object *o)
{
	ANIMATE(3, 0, 3);
	
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->MoveAtDir(o->dir, 0x600);
			o->flags |= FLAG_IGNORE_SOLID;
		}
		case 1:
		{
			o->timer++;
			
			if (o->timer == 16)
			{
				o->flags &= ~FLAG_IGNORE_SOLID;
			}
			else if (o->timer > 16 && o->block[o->dir])
			{
				o->state = 10;
			}
			
			if (o->timer > 20)
			{
				switch(o->dir)
				{
					case LEFT:
						if (o->CenterX() <= player->CenterX() + (32 * CSFI))
							o->state = 10;
					break;
					
					case RIGHT:
						if (o->CenterX() >= player->CenterX() - (32 * CSFI))
							o->state = 10;
					break;
					
					case UP:
						if (o->CenterY() <= player->CenterY() + (32 * CSFI))
							o->state = 10;
					break;
					
					case DOWN:
						if (o->CenterY() >= player->CenterY() - (32 * CSFI))
							o->state = 10;
					break;
				}
			}
		}
		break;
		
		case 10:
		{
			o->y += (4 * CSFI);
			o->ChangeType(OBJ_BUTE_FLYING);
			o->state = 10;	// trigger flight immediately
			
			o->frame = 0;
			o->xinertia = 0;
			o->yinertia = 0;
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_bute_sword(Object *o)
{
	if (run_bute_defeated(o, BUTE_HP))
		return;
	
	switch(o->state)
	{
		case 0:
		{
			o->flags |= (FLAG_SHOOTABLE | FLAG_INVULNERABLE);
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			o->damage = 0;
			o->state = 1;
		}
		case 1:		// lying in wait
		{
			FACEPLAYER;
			
			if (pdistlx(128 * CSFI) && \
				pdistly2(128 * CSFI, 16 * CSFI))
			{
				o->state = 10;
			}
		}
		break;
		
		// wait a moment, then start running at player
		case 10:
		{
			o->flags |= FLAG_INVULNERABLE;
			o->damage = 0;
			o->frame = 0;
			
			o->state = 11;
			o->timer = 0;
		}
		case 11:
		{
			if (++o->timer > 30)
			{
				o->state = 20;
				o->timer = 0;
			}
		}
		break;
		
		// run at player and jump
		case 20:
		{
			o->flags &= ~FLAG_INVULNERABLE;
			o->state = 21;
			FACEPLAYER;
		}
		case 21:
		{
			ANIMATE(3, 0, 1);
			XMOVE(0x400);
			
			if (pdistlx(40 * CSFI))
			{
				o->xinertia /= 2;
				o->yinertia = -0x300;
				
				o->state = 30;
				o->frame = 2;	// sword back, jumping
				sound(SND_ENEMY_JUMP);
			}
			else if (++o->timer > 50)
			{	// timeout, p got away
				o->state = 10;
				o->xinertia = 0;
			}
		}
		break;
		
		// jumping up
		case 30:
		{
			if (o->yinertia > -0x80)
			{
				o->frame = 3;	// sword swipe fwd
				o->damage = 9;
				
				o->state = 31;
				o->timer = 0;
			}
		}
		break;
		
		// swiping sword, in air
		case 31:
		{
			if (++o->timer > 2)
			{
				o->timer = 0;
				o->frame = 4;	// sword down, in front
			}
			
			if (o->blockd && o->yinertia > 0)
			{
				o->xinertia = 0;
				o->damage = 3;
				
				o->state = 32;
				o->timer = 0;
			}
		}
		break;
		
		case 32:
		{
			if (++o->timer > 30)
			{
				o->state = 10;
				o->timer = 0;
			}
		}
		break;
	}
	
	o->yinertia += 0x20;
	LIMITY(0x5ff);
}


void ai_bute_archer(Object *o)
{
	if (run_bute_defeated(o, BUTE_HP))
		return;
	
	switch(o->state)
	{
		case 0:		// waiting for player (when haven't seen him yet)
		{
			if ((o->dir == LEFT  && player->CenterX() < o->CenterX()) || \
				(o->dir == RIGHT && player->CenterX() > o->CenterX()))
			{
				if (pdistlx(320 * CSFI) && pdistly(160 * CSFI))
				{
					o->state = 10;
				}
			}
		}
		break;
		
		// aiming--can track player here for a brief period
		case 10:
		{
			FACEPLAYER;
			
			if (!pdistlx(224 * CSFI) || player->y <= (o->y - (8 * CSFI)))
			{
				o->frame = 4;	// shooting up
				o->timer2 = 1;
			}
			else
			{
				o->frame = 1;	// shooting straight
				o->timer2 = 0;
			}
			
			if (++o->timer > 10)
			{
				o->state = 20;
				o->timer = 0;
			}
		}
		break;
		
		// flashing to fire
		case 20:
		{
			if (o->timer2 == 0)
				ANIMATE(0, 1, 2)
			else
				ANIMATE(0, 4, 5)
			
			if (++o->timer > 30)
				o->state = 30;
		}
		break;
		
		// fire
		case 30:
		{
			o->state = 31;
			o->timer = 0;
			
			Object *arrow = CreateObject(o->CenterX(), o->CenterY(), OBJ_BUTE_ARROW);
			arrow->xinertia = (o->dir == RIGHT) ? 0x600 : -0x600;
			
			if (o->timer2 == 1)		// shooting up
				arrow->yinertia = -0x600;
			
			// frame: arrow away
			o->frame = (o->timer2 == 1) ? 6 : 3;
		}
		case 31:
		{
			if (++o->timer > 30)
			{
				o->state = 40;
				o->frame = 0;
				o->timer = random(50, 150);
			}
		}
		break;
		
		// after fire, and the "woken up" waiting-for-player state
		case 40:
		{
			if (pdistlx(352 * CSFI) && pdistly(240 * CSFI))
			{
				if (--o->timer < 0)
					o->state = 10;	// fire again
			}
			else
			{	// player got away, do nothing until he returns
				o->timer = 150;
			}
		}
		break;
	}
}


void ai_bute_arrow(Object *o)
{
	// check for hit wall/floor etc
	if (o->state < 20)
	{
		if ((o->blockl && o->xinertia < 0) || \
			(o->blockr && o->xinertia > 0) || \
			(o->blocku && o->yinertia <= 0) || \
			(o->blockd && o->yinertia >= 0))
		{
			if (!(o->flags & FLAG_IGNORE_SOLID))
				o->state = 20;
		}
	}
	
	switch(o->state)
	{
		case 0:
		{
			// set appropriate frame for initial direction
			o->sprite = (o->xinertia < 0) ? SPR_BUTE_ARROW_LEFT : SPR_BUTE_ARROW_RIGHT;
			o->frame = (o->yinertia < 0) ? 0 : 2;
			o->state = 1;
		}
		case 1:
		{
			if (++o->timer == 4)
				o->flags &= ~FLAG_IGNORE_SOLID;
			
			if (o->timer > 10)
				o->state = 10;
		}
		break;
		
		case 10:
		{
			o->state = 11;
			
			// slow down a bit (was going real fast from bow)
			o->xinertia *= 3;
			o->xinertia /= 4;
			
			o->yinertia *= 3;
			o->yinertia /= 4;
		}
		case 11:
		{
			o->yinertia += 0x20;
			
			ANIMATE_FWD(10);
			if (o->frame > 4) o->frame = 4;
		}
		break;
		
		case 20:	// hit something
		{
			o->state = 21;
			o->timer = 0;
			o->damage = 0;
			o->xinertia = 0;
			o->yinertia = 0;
		}
		case 21:
		{
			o->timer++;
			
			if (o->timer > 30)
				o->invisible = (o->timer & 2);
				
			if (o->timer > 61)
				o->Delete();
		}
		break;
	}
	
	LIMITY(0x5ff);
}


/*
void c------------------------------() {}
*/


void ai_bute_dying(Object *o)
{

	switch(o->state)
	{
		case 0:
		{
			o->flags &= ~(FLAG_SHOOTABLE | FLAG_IGNORE_SOLID | FLAG_SHOW_FLOATTEXT);
			o->damage = 0;
			o->frame = 0;
			o->animtimer = 0;
			o->state = 1;
			
			o->yinertia = -0x200;
		}
		case 1:
		{
			if (o->blockd && o->yinertia >= 0)
			{
				o->state = 2;
				o->timer = 0;
				o->frame = 1;
			}
		}
		break;
		
		case 2:
		{
			o->xinertia *= 8;
			o->xinertia /= 9;
			
			ANIMATE(3, 1, 2);
			
			if (++o->timer > 50)
				o->DealDamage(10000);
		}
		break;
	}
	
	o->yinertia += 0x20;
	LIMITY(0x5ff);
}

static bool run_bute_defeated(Object *o, int hp)
{
	if (o->hp <= (1000 - hp))
	{
		if (o->type == OBJ_MESA)
		{
			o->ChangeType(OBJ_MESA_DYING);
		}
		else
		{
			o->x -= (4 * CSFI);
			o->y -= (4 * CSFI);
			o->ChangeType(OBJ_BUTE_DYING);
			
			sound(SND_ENEMY_SQUEAK);
			XMOVE(-0x100);
		}
		
		ai_bute_dying(o);
		return 1;
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

void ai_mesa(Object *o)
{
	if (run_bute_defeated(o, MESA_HP))
		return;
	
	switch(o->state)
	{
		case 0:
		{
			o->y += (4 * CSFI);
			o->state = 1;
		}
		case 1:
		{
			ANIMATE(40, 0, 1);
			FACEPLAYER;
			
			if (pdistlx(320 * CSFI) && pdistly(160 * CSFI))
			{
				if (++o->timer > 50)
					o->state = 10;
			}
		}
		break;
		
		case 10:
		{
			o->state = 11;
			o->timer = 0;
			o->frame = 2;	// hand down
			
			int x = o->x + ((o->dir == LEFT) ? (7 * CSFI) : -(7 * CSFI));
			int y = o->y + (10 * CSFI);
			
			o->linkedobject = CreateObject(x, y, OBJ_MESA_BLOCK);
			o->linkedobject->linkedobject = o;
		}
		case 11:
		{
			if (++o->timer > 50)
			{
				o->state = 20;
				o->timer = 0;
				o->frame = 3;	// hand up, throwing
				
				if (o->linkedobject)
				{
					Object *&block = o->linkedobject;
					
					block->y = (o->y - (4 * CSFI));
					block->xinertia = (o->dir == RIGHT) ? 0x400 : -0x400;
					block->yinertia = -0x400;
					block->state = 1;
					
					sound(SND_EM_FIRE);
					block->linkedobject = NULL;
					o->linkedobject = NULL;
				}
			}
		}
		break;
		
		case 20:
		{
			if (++o->timer > 20)
			{	// throw again, if player still near
				o->state = 1;
				o->timer = 0;
			}
		}
		break;
	}
	
	o->yinertia += 0x55;
	LIMITY(0x5ff);
}

void ai_mesa_block(Object *o)
{
	ANIMATE(0, 0, 1);
	
	switch(o->state)
	{
		case 0:		// being held
		{
			if (!o->linkedobject || o->linkedobject->type == OBJ_MESA_DYING)
			{
				o->Delete();
			}
		}
		break;
		
		case 1:		// launched
		{
			if (++o->timer == 4)
				o->flags &= ~FLAG_IGNORE_SOLID;
			
			o->yinertia += 0x2A;
			LIMITY(0x5ff);
			
			if (o->blockd && o->yinertia >= 0)
			{
				sound(SND_BLOCK_DESTROY);
				o->Delete();
			}
		}
		break;
	}
	
	if (o->deleted)
	{
		SmokeClouds(o, 3, 0, 0);
		effect(o->x, o->y, EFFECT_BOOMFLASH);
	}
}

/*
void c------------------------------() {}
*/

void ai_deleet(Object *o)
{
	// trigger counter
	if (o->hp < (1000 - DELEET_HP) && o->state < 2)
	{
		o->state = 2;
		o->timer = 0;
		o->frame = 2;
		
		o->flags |= FLAG_INVULNERABLE;
		sound(SND_CHEST_OPEN);
	}
	
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->x += (TILE_W / 2) * CSFI;
			o->y += (TILE_H / 2) * CSFI;
			
			if (o->dir == LEFT)
				o->y += (8 * CSFI);
			else
				o->x += (8 * CSFI);
		}
		case 1:
		{
			if (o->shaketime)
				o->timer2++;
			else
				o->timer2 = 0;
			
			o->frame = (o->timer2 & 2) ? 1 : 0;
		}
		break;
		
		case 2:
		{
			int counter = -1;
			
			switch(o->timer)
			{
				case 0:		counter = 0; break;	// 5
				case 50:	counter = 1; break;	// 4
				case 100:	counter = 2; break;	// 3
				case 150:	counter = 3; break;	// 2
				case 200:	counter = 4; break;	// 1
				
				case 250:
				{
					o->state = 3;
					o->sprite = SPR_BBOX_PUPPET_1;
					o->invisible = true;
					
					sprites[o->sprite].bbox.x1 = -48;
					sprites[o->sprite].bbox.x2 = 48;
					sprites[o->sprite].bbox.y1 = -48;
					sprites[o->sprite].bbox.y2 = 48;
					o->damage = 12;
					
					quake(10);
					SmokeXY(o->x, o->y, 40, 48, 48);
					
					o->flags &= ~FLAG_SHOOTABLE;
					o->flags &= ~FLAG_INVULNERABLE;
					
					if (o->dir == LEFT)
					{
						int x = (o->x / CSFI) / TILE_W;
						int y = ((o->y / CSFI) - 8) / TILE_H;
						
						map.tiles[x][y] = 0;
						map.tiles[x][y+1] = 0;
					}
					else
					{
						int x = ((o->x / CSFI) - 8) / TILE_W;
						int y = (o->y / CSFI) / TILE_H;
						
						map.tiles[x][y] = 0;
						map.tiles[x+1][y] = 0;
					}
					
				}
				break;
			}
			
			if (counter != -1)
			{
				CreateObject(o->x, o->y - (8 * CSFI), \
						OBJ_COUNTER_BOMB_NUMBER)->frame = counter;
			}
			
			o->timer++;
		}
		break;
		
		case 3:
		{
			// make the explosion sound and drop powerups
			o->Kill();
		}
		break;
	}
}


void ai_rolling(Object *o)
{
	if (o->state == 0)
	{
		// exploding out of blocks (Rollings in Heavy Press area)
		int mx = (o->CenterX() / CSFI) / TILE_W;
		int my = (o->CenterY() / CSFI) / TILE_H;
		
		if (map.tiles[mx][my])
			map_ChangeTileWithSmoke(mx, my, 0, 8);
		
		o->state = 1;
	}
	
	switch(o->dir)
	{
		case LEFT:
		{
			o->xinertia -= 0x40;
			o->yinertia = 0;
			if (o->blockl) o->dir = UP;
		}
		break;
		
		case UP:
		{
			o->yinertia -= 0x40;
			o->xinertia = 0;
			if (o->blocku) o->dir = RIGHT;
		}
		break;
		
		case RIGHT:
		{
			o->xinertia += 0x40;
			o->yinertia = 0;
			if (o->blockr) o->dir = DOWN;
		}
		break;
		
		case DOWN:
		{
			o->yinertia += 0x40;
			o->xinertia = 0;
			if (o->blockd) o->dir = LEFT;
		}
		break;
	}
	
	ANIMATE(1, 0, 2);
	LIMITX(0x400);
	LIMITY(0x400);
}


/*
void c------------------------------() {}
*/

// I don't think it does anything, it seems to be just an invisible marker
// used to bring up the signs when you press DOWN in front of the statues.
// But unlike OBJ_NULL, it can be positioned in-between a tile boundary.
// There's also one on top of the clockroom sign on the Outer Wall.
void ai_statue_base(Object *o)
{
	if (o->state == 0)
	{
		o->state = 1;
		o->sprite = SPR_NULL;
		
		if (o->dir == LEFT)
		{
			o->x += (8 * CSFI);
		}
		else if (o->dir == RIGHT)
		{
			o->y += (16 * CSFI);
		}
	}
}

// frame is passed in the ANP as if it were a dir, and all of these state numbers can
// also be set by the scripts, so they really shouldn't be messed with.
void ai_statue(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->frame = (o->dirparam / 10);
			o->state = 1;
		}
		break;
		
		case 10:
		{
			if (game.flags[o->id1])
			{	// already been destroyed
				o->state = 20;
			}
			else
			{
				o->state = 11;
				o->flags |= FLAG_SHOOTABLE;
			}
		}
		case 11:
		{
			if (o->hp < (1000 - STATUE_HP))
			{
				game.flags[o->id1] = true;
				Object *ns = CreateObject(o->x, o->y, o->type);
				ns->dirparam = (o->frame + 4) * 10;
				ns->PushBehind(OBJ_CURLY_CARRIED_SHOOTING);
				o->Kill();
			}
		}
		break;
		
		case 20:
		{
			o->frame += 4;
			o->state = 1;
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_puppy_ghost(Object *o)
{
	o->timer++;
	
	switch(o->state)
	{
		case 10:	// disappear after talking
		{
			o->state = 11;
			o->timer = 2;
			sound(SND_TELEPORT);
		}
		case 11:
		{
			o->invisible = (o->timer & 2);
			
			if (o->timer > 50)
				o->Delete();
		}
		break;
	}
	
	if ((o->timer % 8) == 1)
	{
		effect(random(o->Left(), o->Right()), o->Bottom(), EFFECT_GHOST_SPARKLE);
	}
}

/*
void c------------------------------() {}
*/




















