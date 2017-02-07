#include "balfrog.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../tsc.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"
#include "../../player.h"
#include "../../map.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


enum Frames
{
	FRAME_STAND				= 0,
	FRAME_CROUCH			= 1,
	FRAME_MOUTH_OPEN		= 2,
	FRAME_MOUTH_OPEN_HURT	= 3,
	
	FRAME_JUMPING			= 0		// on other sprite (SPR_BALFROG_JUMP)
};

enum States
{
	STATE_TRANSFORM			= 20,			// script-triggered: must stay at this value
	STATE_READY				= 10,			// script-triggered: must stay at this value
	STATE_DEATH				= 130,			// script-triggered: must stay at this value
	STATE_FIGHTING			= 100,			// script-triggered: must stay at this value
	
	STATE_JUMPING			= 50,
	
	STATE_OPEN_MOUTH		= 60,
	STATE_SHOOTING			= 70,
	STATE_CLOSE_MOUTH		= 80,
	
	STATE_BIG_JUMP			= 90
};

enum BBox_States
{
	BM_STAND,
	BM_JUMPING,
	BM_MOUTH_OPEN,
	BM_DISABLED
};

#define FROG_START_X			((5 * TILE_W) << CSF)
#define FROG_START_Y			((10 * TILE_H) << CSF)

#define LANDING_SMOKE_COUNT		8
#define LANDING_SMOKE_YTOP		-4

#define DEATH_SMOKE_COUNT		8
#define DEATH_SMOKE_YTOP		-24

// when he lands he spawns frogs from ceiling--
// this is the range of where they should spawn at
#define SPAWN_RANGE_LEFT		4
#define SPAWN_RANGE_RIGHT		16
#define SPAWN_RANGE_TOP			0
#define SPAWN_RANGE_BOTTOM		4

// offset from top and from left or right (depending on direction facing)
// to spawn the balrog 'puppet' when we return to balrog form after being defeated.
#define BALDEATH_X				(12 << CSF)
#define BALDEATH_Y				(44 << CSF)

// twiddle adjustment to get the proper Y coordinate when switching
// between normal and jumping sprites.
#define JUMP_SPRITE_ADJ			(16 << CSF)


INITFUNC(AIRoutines)
{
	ONDEATH(OBJ_BALFROG, ondeath_balfrog);
	ONTICK(OBJ_BALFROG_SHOT, ai_generic_angled_shot);
}

void BalfrogBoss::OnMapEntry(void)
{
	memset(&frog, 0, sizeof(frog));
	
	o = CreateObject(FROG_START_X, FROG_START_Y, OBJ_BALFROG);
	game.stageboss.object = o;
	
	o->hp = 300;
	o->damage = 0;	// damage comes from our bbox puppets, not our own bbox
	o->flags |= FLAG_SHOW_FLOATTEXT;
	
	o->sprite = SPR_BALFROG;
	o->dir = RIGHT;
	o->invisible = true;
	
	// setup the bounding box objects--this boss has an irregular bounding box
	// and so we simulate that by having three invisible objects which are wired
	// to transmit hits to the real Balfrog boss object.
	frog.bboxes.init(o, 3);
	frog.bboxes.set_damage(5);
	frog.bbox_mode = BM_DISABLED;
	
	// now disable being able to hit the Balfrog boss object itself.
	o->flags &= ~FLAG_SHOOTABLE;
	
	objprop[OBJ_BALFROG].xponkill = 1;
	objprop[OBJ_BALFROG].shaketime = 9;
}

/*
void c------------------------------() {}
*/

void BalfrogBoss::Run()
{
	if (!o) return;
	
	// each subroutine handles a subset of the frog's finite state machine
	RunFighting();
	RunJumping();
	RunShooting();
	
	RunEntryAnim();
	RunDeathAnim();
	
	if (o)	// because RunDeathAnim destroys o at end of fight
	{
		o->yinertia += 0x40;
		// don't limit upwards inertia or Big Jump will fail
		if (o->yinertia > 0x5FF) o->yinertia = 0x5FF;
		
		// link our "irregular" bbox (actually composed of multiple "puppet" bboxes)
		// to our real object.
		frog.bboxes.transmit_hits();
		frog.bboxes.place(&call_place_bboxes, this);
	}
}

/*
void c------------------------------() {}
*/

static void call_place_bboxes(void *balfrog)
{
	((BalfrogBoss *)balfrog)->place_bboxes();
}

void BalfrogBoss::place_bboxes()
{
	#define set_bbox	frog.bboxes.set_bbox
	
	// I got these coordinates by drawing rectangles over the
	// sprites in Photoshop. These are for the right-facing frame
	// and are automatically flipped if the object is facing left.
	switch(frog.bbox_mode)
	{
		case BM_STAND:
			set_bbox(0,  5, 28, 50, 36,  FLAG_INVULNERABLE);	// body
			set_bbox(1,  37, 4, 38, 36,  FLAG_INVULNERABLE);	// head
		break;
		
		case BM_JUMPING:
			set_bbox(0,  12, 29, 41, 47, FLAG_INVULNERABLE);	// body
			set_bbox(1,  30, 3, 43, 35,  FLAG_INVULNERABLE);	// head
		break;
		
		case BM_MOUTH_OPEN:
			set_bbox(0,	 8, 22, 38, 42,  FLAG_INVULNERABLE);	// backside
			set_bbox(1,  46, 54, 13, 10, FLAG_INVULNERABLE);	// feet below mouth
			set_bbox(2,  46, 15, 21, 39, FLAG_SHOOTABLE);		// mouth target
		break;
	}
}

/*
void c------------------------------() {}
*/

// the "master" start state, we can always return here eventually from
// all the other states. The script also sets this state to start the fight.
void BalfrogBoss::RunFighting()
{
	switch(o->state)
	{
		case STATE_FIGHTING:
		{
			o->frame = FRAME_STAND;
			frog.bbox_mode = BM_STAND;
			
			o->state++;
			o->timer = 0;
			o->xinertia = 0;
		}
		case STATE_FIGHTING+1:
		{
			o->timer++;
			
			// prepare to jump
			if (o->timer < 50)  o->frame = FRAME_STAND;
			if (o->timer == 50) o->frame = FRAME_CROUCH;
			if (o->timer == 60) o->frame = FRAME_STAND;
			
			// jump
			if (o->timer > 64)
				o->state = STATE_JUMPING;
		}
		break;
	}
}

// handles the hopping and the "big jump" attack
// (a straight up/down jump after every 3rd attack that spawns tons of frogs).
void BalfrogBoss::RunJumping()
{
	switch(o->state)
	{
		case STATE_JUMPING:
		{
			sound(SND_FUNNY_EXPLODE);
			
			SetJumpingSprite(true);
			o->yinertia = -0x400;
			
			XMOVE(0x200);
			o->timer = 0;
			o->state++;
		}
		case STATE_JUMPING+1:
		{
			// turn around at walls
			if (o->dir == LEFT && o->blockl) o->dir = RIGHT;
			if (o->dir == RIGHT && o->blockr) o->dir = LEFT;
			
			// landed?
			if (++o->timer > 3 && o->blockd)
			{
				quake(30);
				SetJumpingSprite(false);
				
				// passed player? turn around and fire!
				if ((o->dir == RIGHT && o->x >= player->x) || \
					(o->dir == LEFT && o->x <= player->x))
				{
					o->dir ^= 1;
					o->state = STATE_OPEN_MOUTH;
				}
				else
				{
					o->state = STATE_FIGHTING;
				}
				
				// shake a small frog loose from the ceiling on every landing
				SpawnFrogs(OBJ_MINIFROG, 1);
				SpawnSmoke(LANDING_SMOKE_COUNT, LANDING_SMOKE_YTOP);
			}
		}
		break;
		
		case STATE_BIG_JUMP:
		{
			o->state++;
			o->timer = 0;
			o->xinertia = 0;
		}
		case STATE_BIG_JUMP+1:		// animation of preparing to jump
		{
			o->timer++;
			
			if (o->timer < 50)  o->frame = FRAME_STAND;
			if (o->timer == 50) o->frame = FRAME_CROUCH;
			if (o->timer == 70) o->frame = FRAME_STAND;
			
			if (o->timer > 74)
			{
				o->state++;
				
				SetJumpingSprite(true);
				o->yinertia = -0xA00;
			}
		}
		break;
		
		case STATE_BIG_JUMP+2:		// in air, waiting to hit ground
		{
			// pass through ceiling at edges
			if (o->y <= MAPY(8))
				o->flags |= FLAG_IGNORE_SOLID;
			else
				o->flags &= ~FLAG_IGNORE_SOLID;
			
			if (++o->timer > 3 && o->blockd)
			{
				o->flags &= ~FLAG_IGNORE_SOLID;
				SetJumpingSprite(false);
				quake(60);
				
				SpawnFrogs(OBJ_MINIFROG, 6);
				SpawnFrogs(OBJ_FROG, 2);
				SpawnSmoke(LANDING_SMOKE_COUNT, LANDING_SMOKE_YTOP);
				
				// player ran under us? turn around and fire!
				if ((o->dir == RIGHT && o->x >= player->x) || \
					(o->dir == LEFT && o->x <= player->x))
				{
					o->state = STATE_OPEN_MOUTH;
				}
				else
				{
					o->state = STATE_FIGHTING;
				}
				
				FACEPLAYER;
			}
		}
		break;
	}
}


void BalfrogBoss::RunShooting()
{
	switch(o->state)
	{
		case STATE_OPEN_MOUTH:		// open mouth and fire shots
		{
			o->frame = FRAME_STAND;
			o->timer = 0;
			o->state++;
		}
		case STATE_OPEN_MOUTH+1:
		{
			o->xinertia *= 8;
			o->xinertia /= 9;
			
			o->timer++;
			
			if (o->timer == 50)
				o->frame = FRAME_CROUCH;
			
			if (o->timer > 54)
			{
				o->state = STATE_SHOOTING;
				o->timer = 0;
				
				o->frame = FRAME_MOUTH_OPEN;
				frog.bbox_mode = BM_MOUTH_OPEN;
				
				frog.orighp = o->hp;
				frog.shots_fired = 0;
			}
		}
		break;
		
		case STATE_SHOOTING:
		{
			frog.bbox_mode = BM_MOUTH_OPEN;
			o->frame = FRAME_MOUTH_OPEN;
			o->xinertia *= 10;
			o->xinertia /= 11;
			
			if (o->shaketime)
			{
				if (++frog.shakeflash & 2)
					o->frame = FRAME_MOUTH_OPEN_HURT;
			}
			else
			{
				frog.shakeflash = 0;
			}
			
			if (++o->timer > 16)
			{
				o->timer = 0;
				
				EmFireAngledShot(o, OBJ_BALFROG_SHOT, 16, 0x200);
				sound(SND_EM_FIRE);
				
				if (++frog.shots_fired > 10 || o->hp < (frog.orighp - 90))
				{
					o->frame = FRAME_CROUCH;
					o->state = STATE_CLOSE_MOUTH;
					frog.bbox_mode = BM_STAND;
					o->timer = 0;
				}
			}
		}
		break;
		
		case STATE_CLOSE_MOUTH:
		{
			o->frame = FRAME_CROUCH;
			
			if (++o->timer > 10)
			{
				o->timer = 0;
				o->frame = FRAME_STAND;
				
				if (++frog.attackcounter >= 3)
				{	// big jump after every 3rd attack
					frog.attackcounter = 0;
					o->state = STATE_BIG_JUMP;
				}
				else
				{
					o->state = STATE_FIGHTING;
				}
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

// the animation where we first appear
// both these states are triggered by the script
void BalfrogBoss::RunEntryAnim()
{
	switch(o->state)
	{
		// transforming from Balrog
		// the flicker is calibrated to be interlaced exactly out-of-phase
		// with Balrog's flicker, which is entirely separate.
		case STATE_TRANSFORM:
		{
			o->timer = 0;
			o->frame = FRAME_MOUTH_OPEN;
			o->state++;
		}
		case STATE_TRANSFORM+1:
		{
			o->timer++;
			o->invisible = (o->timer & 2) ? true : false;
		}
		break;
		
		// transformation complete: puff away balrog, and appear solid now
		case STATE_READY:
		{
			SmokeXY(o->x + ((sprites[SPR_BALROG_GREEN].w / 2) << CSF), \
				o->y + (48 << CSF),
				8, 16, 16);
			
			o->state++;
			o->frame = FRAME_MOUTH_OPEN;
		}
		break;
	}
}


void BalfrogBoss::RunDeathAnim()
{
	switch(o->state)
	{
		case STATE_DEATH:			// BOOM!
		{
			SetJumpingSprite(false);
			o->frame = FRAME_MOUTH_OPEN;
			
			sound(SND_BIG_CRASH);
			o->xinertia = 0;
			o->timer = 0;
			o->state++;
			
			SpawnSmoke(DEATH_SMOKE_COUNT, DEATH_SMOKE_YTOP);
		}
		case STATE_DEATH+1:			// shaking with mouth open
		{
			o->timer++;
			if ((o->timer % 5) == 0)
			{
				SpawnSmoke(1, DEATH_SMOKE_YTOP);
			}
			
			// at a glance it might seem like this has it alternate
			// slowly between 2 X coordinates, but in fact, it
			// alternates quickly between 3.
			o->x += (o->timer & 2) ? (1 << CSF) : (-1 << CSF);
			
			if (o->timer > 100)
			{
				o->timer = 0;
				o->state++;
			}
		}
		break;
		
		case STATE_DEATH+2:			// begin flashing back and forth between frog and balrog
		{
			// spawn balrog puppet
			frog.balrog = CreateObject(0, o->y+BALDEATH_Y, OBJ_BALROG);
			frog.balrog->state = 500;	// tell him to give us complete control
			frog.balrog->dir = o->dir;
			frog.balrog->frame = 5;
			
			if (o->dir == RIGHT)
			{
				frog.balrog->x = (o->x + BALDEATH_X);
			}
			else
			{
				frog.balrog->x = o->x + o->Width();	// not the same as o->Right()
				frog.balrog->x -= frog.balrog->Width();
				frog.balrog->x -= BALDEATH_X;
			}
			
			o->state++;
		}
		case STATE_DEATH+3:		// flashing
		{
			o->timer++;
			
			if ((o->timer % 9) == 0)
				SpawnSmoke(1, DEATH_SMOKE_YTOP);
			
			if (o->timer <= 150)
			{
				o->invisible = (o->timer & 2);
				frog.balrog->invisible = !(o->timer & 2);
			}
			
			if (o->timer > 156)
			{
				o->timer = 0;
				o->state++;
			}
		}
		break;
		
		case STATE_DEATH+4:		// balrog falling to ground
		{
			// should start to move exactly when timer hits 160
			//
			// 10 frames until starts to fall
			// 14 frames until changes to landed frame
			frog.balrog->yinertia += 0x40;
			
			if (frog.balrog->blockd)
			{
				frog.balrog->frame = 2;
				if (++o->timer > 30)
				{
					frog.balrog->frame = 3;
					o->state++;
				}
			}
		}
		break;
		
		case STATE_DEATH+5:		// balrog flying away
		{
			if (++o->timer > 30)
			{
				// it's all over, destroy ourselves and clean up
				frog.balrog->yinertia = -0xA00;
				frog.balrog->flags |= FLAG_IGNORE_SOLID;
				
				if (frog.balrog->y < -(100 << CSF))
				{
					frog.balrog->Delete();
					frog.bboxes.destroy();
					
					o->Delete();
					o = game.stageboss.object = NULL;
					return;
				}
			}
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

void ondeath_balfrog(Object *o)
{
	o->flags &= ~FLAG_SHOOTABLE;
	StartScript(1000);
}

/*
void c------------------------------() {}
*/

// shake loose frogs from the ceiling
void BalfrogBoss::SpawnFrogs(int objtype, int count)
{
Object *child;

	for(int i=0;i<count;i++)
	{
		int x = random(SPAWN_RANGE_LEFT, SPAWN_RANGE_RIGHT);
		int y = random(SPAWN_RANGE_TOP, SPAWN_RANGE_BOTTOM);
		
		child = CreateObject((x*TILE_W)<<CSF, (y*TILE_H)<<CSF, objtype);
		child->dir = DOWN;	// allow fall through ceiling
	}
}

// spawn the smoke clouds from landing after a jump
// or during the death sequence.
void BalfrogBoss::SpawnSmoke(int count, int ytop)
{
Object *smoke;

	for(int i=0;i<count;i++)
	{
		int x = random(o->Left() + (4 << CSF), o->Right() - (4<<CSF));
		int y = o->Bottom() + random(ytop<<CSF, 4<<CSF);
		
		smoke = CreateObject(x, y, OBJ_SMOKE_CLOUD);
		smoke->xinertia = random(-0x155, 0x155);
		smoke->yinertia = random(-0x600, 0);
	}
}


// switches on and off the jumping frame/sprite
void BalfrogBoss::SetJumpingSprite(bool enable)
{
	if (enable != (o->sprite == SPR_BALFROG_JUMP))
	{
		if (enable)
		{
			o->sprite = SPR_BALFROG_JUMP;
			o->frame = FRAME_JUMPING;
			o->y -= JUMP_SPRITE_ADJ;
			
			frog.bbox_mode = BM_JUMPING;
		}
		else
		{
			o->sprite = SPR_BALFROG;
			o->frame = FRAME_STAND;
			o->y += JUMP_SPRITE_ADJ;
			
			frog.bbox_mode = BM_STAND;
		}
	}
}


