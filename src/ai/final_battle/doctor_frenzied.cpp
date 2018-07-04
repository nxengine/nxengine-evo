#include "doctor_frenzied.h"
#include "doctor.h"
#include "doctor_common.h"

#include "../stdai.h"
#include "../ai.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../player.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"
#include "../../graphics/graphics.h"
#include "../../graphics/tileset.h"
#include "../../graphics/sprites.h"


enum STATES
{
	// 0-9 are used by in-game scripts during intro
	// 500+ is triggered for defeat sequence
	
	STATE_BASE				= 10,		// fall; once on ground for a moment pick next attack
	
	STATE_JUMP				= 20,		// jumps at player
	STATE_JUMP_WITH_GP		= 30,		// taller jump and enable "ground pound"
	STATE_IN_AIR			= 40,		// in air
	STATE_IN_AIR_WITH_GP	= 50,		// in air and will "ground pound" if passes over player
	STATE_LANDED			= 60,		// landed from a jump
	
	STATE_RED_DASH			= 70,		// red dash; try to hit player with elbow
	STATE_MEGA_BATS			= 80,		// arms out & spawn mega bats
	
	STATE_TELEPORT			= 90,		// teleport away and reappear over players head
	
	STATE_DEFEATED			= 500,		// script constant
	STATE_DISSOLVE			= 510		// also scripted
};

#define DAMAGE_NORMAL		5
#define DAMAGE_RED_DASH		10

// the Doctor repeats this series of attacks over and over,
// when he reaches the -1, it starts over at the beginning.
static const int attack_pattern[] =
{
	STATE_JUMP_WITH_GP,
	STATE_TELEPORT,
	STATE_RED_DASH,
	STATE_JUMP,
	STATE_JUMP,
	STATE_RED_DASH,
	STATE_TELEPORT,
	STATE_MEGA_BATS,
	STATE_JUMP_WITH_GP,
	STATE_JUMP,
	STATE_JUMP,
	-1
};

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BOSS_DOCTOR_FRENZIED, ai_boss_doctor_frenzied);
	ONTICK(OBJ_DOCTOR_BAT, ai_doctor_bat);
}

/*
void c------------------------------() {}
*/

static void run_jumps(Object *o)
{
	
	switch(o->state)
	{
		// jump at player
		case STATE_JUMP:
		{
			o->frame = 3;
			FACEPLAYER;
			
			if (++o->timer > 20)
			{
				o->state = STATE_IN_AIR;
				o->frame = 4;
				o->animtimer = 0;
				
				o->yinertia = -0x600;
				XMOVE(0x400);
			}
		}
		break;
		
		// slight taller jump with no delay, and can "ground pound"
		case STATE_JUMP_WITH_GP:
		{
			FACEPLAYER;
			
			o->state = STATE_IN_AIR_WITH_GP;
			o->frame = 4;
			o->animtimer = 0;
			
			o->yinertia = -0x800;
			XMOVE(0x400);
		}
		break;
		
		
		case STATE_IN_AIR:			// in air (normal)
		case STATE_IN_AIR_WITH_GP:	// in air; can "ground pound" if we pass over player
		{
			ANIMATE(1, 4, 5);
			o->yinertia += 0x40;
			
			if (o->state == STATE_IN_AIR_WITH_GP)
			{
				if (pdistlx(8 * CSFI) && player->y >= o->y)
				{
					o->xinertia = 0;
					o->yinertia = 0x5ff;
					o->state = STATE_IN_AIR;
				}
			}
			else
			{
				FACEPLAYER;
			}
			
			if (o->blockd && o->yinertia > 0)
				o->state = STATE_LANDED;
		}
		break;
		
		
		// landed from jump
		case STATE_LANDED:
		{
			o->frame = 3;
			quake(10);
			
			o->timer = 0;
			o->state++;
		}
		case STATE_LANDED+1:
		{
			o->yinertia += 0x80;
			
			o->xinertia *= 7;
			o->xinertia /= 8;
			
			if (++o->timer > 10)
				o->state = STATE_BASE;
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

// flashing red elbow dash
static void run_red_dash(Object *o)
{
	switch(o->state)
	{
		// prepare to dash
		case STATE_RED_DASH:
		{
			o->frame = 3;	// crouch down; look mean
			o->flags |= (FLAG_SOLID_MUSHY | FLAG_INVULNERABLE);
			
			o->timer = 0;
			o->state++;
		}
		case STATE_RED_DASH+1:
		{
			if (++o->timer > 20)
			{
				o->frame = 7;		// elbow-out dash frame
				o->timer = 0;
				o->state++;
				
				sound(SND_FUNNY_EXPLODE);
				XMOVE(0x5ff);
				
				o->damage = DAMAGE_RED_DASH;
				o->flags |= FLAG_NOREARTOPATTACK;
				o->yinertia = 0;	// he does not fall during dash
			}
		}
		break;
		
		// doing red dash
		case STATE_RED_DASH+2:
		{
			// flash red
			o->timer++;
			o->frame = (o->timer & 2) ? 7 : 8;
			
			// time to stop?
			if ((o->blockl && o->xinertia < 0) || \
				(o->blockr && o->xinertia > 0) || \
				o->timer > 30)
			{
				if (o->timer > 30)		// stopped because timeout
					o->state++;
				else					// stopped because hit a wall
					o->state = STATE_JUMP;
				
				o->flags &= ~(FLAG_SOLID_MUSHY | FLAG_NOREARTOPATTACK | FLAG_INVULNERABLE);
				o->damage = DAMAGE_NORMAL;
				o->timer = 0;
			}
		}
		break;
		
		// dash ending due to timeout
		case STATE_RED_DASH+3:
		{
			o->yinertia += 0x80;
			o->frame = 3;
			
			o->xinertia *= 7;
			o->xinertia /= 8;
			
			if (++o->timer > 10)
				o->state = STATE_BASE;
		}
		break;
	}
}


// arms thrust out, spawn oodles of bouncing bats
static void run_mega_bats(Object *o)
{

	switch(o->state)
	{
		case STATE_MEGA_BATS:
		{
			o->state++;
			o->timer = 0;
		}
		case STATE_MEGA_BATS+1:
		{
			o->frame = 6;
			o->timer++;
			
			if (o->timer > 20 && (o->timer % 3) == 1)
			{
				Object *bat = CreateObject(o->x + (8 * CSFI), \
										   o->y - (4 * CSFI), OBJ_DOCTOR_BAT);
				
				bat->xinertia = random(0x400, 0x800);
				bat->yinertia = random(-0x200, 0x200);
				bat->dir = o->dir;
				
				if (o->dir == LEFT)
				{
					bat->x -= (16 * CSFI);
					bat->xinertia = -bat->xinertia;
				}
				
				sound(SND_EM_FIRE);
			}
			
			if (o->timer > 90)
				o->state = STATE_BASE;
		}
		break;
	}
}


// teleport away and return
static void run_teleport(Object *o)
{
	switch(o->state)
	{
		case STATE_TELEPORT:
		{
			o->flags &= ~FLAG_SHOOTABLE;
			o->damage = 0;
			
			o->state++;
			dr_tp_out_init(o);
		}
		case STATE_TELEPORT+1:
		{
			if (dr_tp_out(o))
			{
				o->state++;
				o->timer = 0;
				
				// mark the location above player's head where we'll reappear
				o->xmark = player->x;
				o->ymark = player->y - (32 * CSFI);
				
				// don't be fooled into going off bounds of map
				#define TP_X_MIN	((4 * TILE_W) * CSFI)
				#define TP_X_MAX	((36 * TILE_W) * CSFI)
				#define TP_Y_MIN	((4 * TILE_W) * CSFI)
				
				if (o->xmark < TP_X_MIN) o->xmark = TP_X_MIN;
				if (o->xmark > TP_X_MAX) o->xmark = TP_X_MAX;
				if (o->ymark < TP_Y_MIN) o->ymark = TP_Y_MIN;
			}
		}
		break;
		
		// invisible...waiting to reappear
		case STATE_TELEPORT+2:
		{
			if (++o->timer > 40)
			{
				o->x = o->xmark;
				o->y = o->ymark;
				o->frame = 4;
				
				FACEPLAYER;
				
				o->state++;
			}
		}
		break;
		
		// reappear
		case STATE_TELEPORT+3:
		{
			dr_tp_in_init(o);
			o->yinertia = 0;
			o->state++;
		}
		case STATE_TELEPORT+4:
		{
			if (dr_tp_in(o))
			{
				o->flags |= FLAG_SHOOTABLE;
				o->damage = DAMAGE_NORMAL;
				
				o->xinertia = 0;
				o->yinertia = -0x200;
				o->state = STATE_IN_AIR;
			}
		}
		break;
	}
	
}


/*
void c------------------------------() {}
*/

// initilization/transformation animation and "prepare to fight" states.
// this are of course all script-triggered and need to stay constant.
static void run_init(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			// put ourselves at correct position over the other doctor
			o->dir = (crystal_xmark > player->x) ? LEFT : RIGHT;
			
			o->x = crystal_xmark + ((o->dir == RIGHT) ? (6 * CSFI) : -(6 * CSFI));
			o->y = crystal_ymark;
			
			// make sure we're front of other doctor
			o->BringToFront();
			// make sure front crystal is in front of us
			crystal_tofront = true;
			
			// because we moved our x/y directly
			o->UpdateBlockStates(ALLDIRMASK);
			o->state = 1;
		}
		case 1:		// appearing/transforming
		{
			o->yinertia += 0x80;
			
			o->timer++;
			o->frame = (o->timer & 2) ? 0 : 3;
		}
		break;
		
		// standing idle & panting
		// this state doesn't seem to ever be used;
		// AFAIK can only be triggered by modifying the script.
		case 5:
		{
			o->frame = 1;
			o->animtimer = 0;
			o->state = 6;
		}
		case 6:
		{
			o->yinertia += 0x80;
			ANIMATE(30, 1, 2);
		}
		break;
		
		case 7: 	// prepare-to-fight pause
		{
			o->state = 8;
			o->timer = 0;
			o->frame = 3;
		}
		case 8:
		{
			o->yinertia += 0x40;
			
			if (++o->timer > 40)
				o->state = STATE_BASE;
		}
		break;
	}
}


// defeated states and animation
static void run_defeat(Object *o)
{
	switch(o->state)
	{
		// Defeated!
		case STATE_DEFEATED:
		{
			KillObjectsOfType(OBJ_DOCTOR_BAT);
			o->flags &= ~FLAG_SHOOTABLE;
			o->damage = 0;
			o->frame = 4;
			o->xinertia = 0;
			o->state++;
		}
		case STATE_DEFEATED+1:		// wait till we hit ground
		{
			o->yinertia += 0x20;
			
			if (o->blockd && o->yinertia > 0)
			{
				o->state++;
				o->timer = 0;
				o->xmark = o->x;
				FACEPLAYER;
			}
		}
		break;
		
		// shaking (script tells us when to stop)
		case STATE_DEFEATED+2:
		{
			o->frame = 9;
			o->timer++;
			
			o->x = o->xmark;
			if (!(o->timer & 2)) o->x += (1 * CSFI);
		}
		break;
		
		
		// dissolve into red energy
		// we already did this once before; think Pooh Black.
		case STATE_DISSOLVE:
		{
			o->frame = 9;
			o->x = o->xmark;
			
			o->ResetClip();
			o->clip_enable = true;
			
			o->state++;
			o->timer = 0;
		}
		case STATE_DISSOLVE+1:
		{
			o->timer++;
			
			// shaking
			o->x = o->xmark;
			if (!(o->timer & 2)) o->x += (1 * CSFI);
			
			game.quaketime = 2;
			
			// sound
			if ((o->timer % 6) == 3)
				sound(SND_FUNNY_EXPLODE);
			
			// move energy spawn point
			if (++o->timer2 >= 8)
			{
				o->timer2 = 0;
				
				o->clipy1++;
				if (o->clipy1 >= sprites[o->sprite].h)
					o->invisible = true;
			}
			
			// spawn copious amount of energy
			for(int i=0;i<3;i++)
			{
				int x, y;
				
				x = o->x + (random(-16, 16) * CSFI);
				y = (o->y - o->DrawPointY()) + (o->clipy1 * CSFI);
				
				Object *drip = CreateObject(x, y, OBJ_RED_ENERGY);
				
				drip->xinertia = random(-0x200, 0x200);
				drip->yinertia = random(-0x400, 0);
				drip->angle = DOWN;
				// otherwise during the last few frames they'll get stuck in the floor
				// (they still delete themselves once they hit the floor, just are
				// able to come up out of it then back down during last few moments).
				drip->flags |= FLAG_IGNORE_SOLID;
			}
			
			// he doesn't take up the entire height of the sprite,
			// so we stop a little bit early.
			if (o->clipy1 >= 44)
			{
				o->invisible = true;
				o->frame = 0;
				o->state++;
			}
		}
		break;
		
		// script: crystal up and away
		case 520:
		{
			crystal_ymark = -(128 * CSFI);
		}
		break;
	}
}




/*
void c------------------------------() {}
*/

// this is a lesser-seen attack in which he pushes you away amongst
// a shower of red sparks. To trigger it, immediately after he lands
// you must walk directly up to him and deal more than 20 damage.
static void do_redsplode(Object *o)
{
	// arms out full
	o->frame = 6;
	FACEPLAYER;
	
	player->yinertia = -0x400;
	player->xinertia = (o->x > player->x) ? -0x5ff : 0x5ff;
	
	hurtplayer(5);
	quake(10);
	
	// big shower of red energy
	for(int i=0;i<100;i++)
	{
		int x = o->x + (random(-16, 16) * CSFI);
		int y = o->y + (random(-16, 16) * CSFI);
		
		Object *spark = CreateObject(x, y, OBJ_RED_ENERGY);
		
		spark->xinertia = random(-0x600, 0x600);
		spark->yinertia = random(-0x600, 0x600);
		spark->angle = DOWN;
	}
}

/*
void c------------------------------() {}
*/

// the red energy that oozes off of him during most of the battle
static void run_red_drip(Object *o)
{
	if (random(0, 3) == 2)
	{
		int x = o->x + (random(-16, 16) * CSFI);
		int y = o->y + (random(-8, 4) * CSFI);
		
		Object *drip = CreateObject(x, y, OBJ_RED_ENERGY);
		drip->xinertia = o->xinertia;
		drip->angle = DOWN;
	}
}


void ai_boss_doctor_frenzied(Object *o)
{
	//AIDEBUG;
	
	switch(o->state)
	{
		// fight begin/base state
		case STATE_BASE:
		{
			o->flags |= FLAG_SHOOTABLE;
			o->flags &= ~FLAG_INVULNERABLE;
			
			o->xinertia = 0;
			o->frame = 1;
			
			o->timer = 0;
			o->animtimer = 0;
			
			o->savedhp = o->hp;
			o->state++;
		}
		case STATE_BASE+1:
		{
			o->yinertia += 0x80;
			FACEPLAYER;
			
			// select frame
			if (o->frame == 6)	// arms out
			{
				// must have done "redsplode" attack; leave alone for the duration
			}
			else if (!o->blockd)
			{	// falling
				o->frame = 4;
			}
			else
			{	// panting animation
				ANIMATE(10, 1, 2);
				
				// "redsplode" attack if possible
				if ((o->savedhp - o->hp) > 20)
				{
					if (pdistlx(48 * CSFI) && player->blockd)
						do_redsplode(o);
				}
			}
			
			// after a moment select next attack in the loop
			if (++o->timer > 30 || (o->savedhp - o->hp) > 20)
			{
				o->state = attack_pattern[o->timer2];
				o->timer = 0;
				
				// move to next state
				if (attack_pattern[++o->timer2] == -1)
					o->timer2 = 0;
			}
		}
		break;
	}
	
	run_jumps(o);
	
	run_red_dash(o);
	run_mega_bats(o);
	run_teleport(o);
	run_init(o);
	run_defeat(o);
	
	if (o->state < STATE_DISSOLVE)
		run_red_drip(o);
	
	// set crystal follow position
	// still set it on first 2 DEFEATED states (falling to ground)
	// but not after that (alert: this seems pretty damn bug prone, fixme)
	if (o->state >= STATE_BASE && o->state <= STATE_DEFEATED+1)
	{
		if (o->invisible)	// in middle of teleport: after tp out, before tp in
		{
			crystal_xmark = o->xmark;
			crystal_ymark = o->ymark;
		}
		else
		{
			crystal_xmark = o->x;
			crystal_ymark = o->y;
		}
	}
	
	if (o->yinertia > 0x5ff)
		o->yinertia = 0x5ff;
}




/*
void c------------------------------() {}
*/

void ai_doctor_bat(Object *o)
{
	ANIMATE(2, 0, 2);
	
	if ((o->blockl && o->xinertia < 0) || \
		(o->blockr && o->xinertia > 0))
	{
		o->dir ^= 1;
		o->xinertia = -o->xinertia;
	}
	else if ((o->blocku && o->yinertia < 0) || \
		(o->blockd && o->yinertia > 0))
	{
		o->yinertia = -o->yinertia;
	}
}

























