#include "sisters.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../screeneffect.h"
#include "../../graphics/graphics.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


// mainstates
#define STATE_CIRCLE_RIGHT		100		// circling right
#define STATE_CIRCLE_LEFT		110		// circling left
#define STATE_MEGAFIRING		120		// runs pause and circling during low-hp megafire
#define STATE_DEFEATED			1000	// script-triggered; must be constant
#define STATE_DEFEATED_CRASH	1010
#define STATE_STARFLASH			1020

// head states
#define STATE_HEAD_CLOSED		10		// closed, but periodically opens by itself and fires
#define STATE_HEAD_OPEN			20		// mouth open, can be shot
#define STATE_HEAD_FIRE			30		// actually firing
#define STATE_HEAD_MEGAFIRE		40		// firing a whole lot during low-hp
#define STATE_HEAD_BIT_TONGUE	50		// bit tongue after got shot
#define STATE_HEAD_DEFEATED		100		// defeated, eyes closed

// body states
#define STATE_BODY_NOMOVE		10		// do nothing but animate
#define STATE_BODY_FACE_PLAYER	20		// follow angle set by main and face player
#define STATE_BODY_FACE_CENTER	30		// follow angle set by main and face center
#define STATE_BODY_LOCK_DIR		40		// follow angle set by main but don't change direction

#define SISTERS_HP				500		// total HP
#define SISTERS_ATTACK2_HP		300		// HP below which we can do the lots-of-fire attack
#define SISTERS_DAMAGE			10		// how much damage you can take if you run into one

// coordinates of per-frame bboxes for each frame of the head object.
// these are all for the right-facing frame and are automatically flipped-over
// at runtime if the dragon is facing left.
// I would like to make this a feature of the .sif at a later date that could be
// enabled on a per-sprite basis via a checkbox in sprprop.
static struct
{
	int x1, y1, x2, y2;
	uint32_t flags;
}
head_bboxes[] =
{
	{ 15, 1, 38, 26,  FLAG_INVULNERABLE },	// closed
	{ 15, 4, 36, 28,  FLAG_INVULNERABLE },	// partway open
	{ 14, 6, 31, 27,  FLAG_SHOOTABLE },		// open (shootable)
	{ 16, 6, 37, 26,  FLAG_SHOOTABLE },		// bit tongue (shootable)
	{ 17, 6, 38, 25,  0 }					// eyes closed (shots pass through)
};

/*
void c------------------------------() {}
*/

void SistersBoss::OnMapEntry()
{
int i;

/*stat("%d %d %d %d",
	sprites[SPR_SISTERS_HEAD].bbox.x1,
	sprites[SPR_SISTERS_HEAD].bbox.y1,
	sprites[SPR_SISTERS_HEAD].bbox.x2,
	sprites[SPR_SISTERS_HEAD].bbox.y2
	); exit(1);*/
	
	main = CreateObject(((10*TILE_W)) * CSFI, ((8*TILE_H)-4) * CSFI, OBJ_SISTERS_MAIN);
	main->invisible = true;
	
	for(i=0;i<NUM_SISTERS;i++)
	{
		body[i] = CreateObject((64 * CSFI) + (50 * CSFI)*i, 80 * CSFI, OBJ_SISTERS_BODY);
		body[i]->sprite = SPR_SISTERS_BODY;
		body[i]->damage = SISTERS_DAMAGE;
	}
	
	for(i=0;i<NUM_SISTERS;i++)
	{
		head[i] = CreateObject((64 * CSFI) + (50 * CSFI)*i, 64 * CSFI, OBJ_SISTERS_HEAD);
		head[i]->hp = 1000;
		head[i]->damage = SISTERS_DAMAGE;
		
		// we give each dragon a different but identical-looking head sprite
		// so that we can hack their bboxes individually.
		head[i]->sprite = SPR_SISTERS_HEAD_1 + i;
	}
	
	objprop[OBJ_SISTERS_HEAD].shaketime = 8;
	objprop[OBJ_SISTERS_HEAD].hurt_sound = NXE::Sound::SFX::SND_ENEMY_HURT_COOL;
	
	mainangle = 0;
	if (widescreen)
	{
	    main->xmark = 180*2;
	    main->ymark = 61*2;
	}
	else
	{
	    main->xmark = 180;
	    main->ymark = 61;
	}
	main->timer2 = random(700, 1200);
	main->hp = 500;
	
	main->id2 = 1000;
	main->flags |= FLAG_SCRIPTONDEATH;
	
	game.stageboss.object = main;
}

void SistersBoss::OnMapExit()
{
	main = NULL;
	game.stageboss.object = NULL;
}

/*
void c------------------------------() {}
*/

void SistersBoss::Run(void)
{
int i;

	Object *o = main;
	if (!o) return;
	
	//debug("main->state: %d", o->state);
	//debug("main->timer: %d", o->timer);
	//DebugCrosshair(o->xmark, o->ymark, 255,255,255);
	
	switch(o->state)
	{
		case 20:	// fight begin (script-triggered)
		{
        	if (widescreen)
        	{
        	    main->xmark = 180;
        	    main->ymark = 61;
        	}

			if (++o->timer > 68)
			{
				o->xmark = 112;		// bodies zoom onto screen via force of their interpolation
				o->timer = 0;
				
				o->state = STATE_CIRCLE_RIGHT;			// main begins turning angle
				SetHeadStates(STATE_HEAD_CLOSED);		// periodically opens/fires by itself
				SetBodyStates(STATE_BODY_FACE_PLAYER);	// enable body movement
			}
		}
		break;
		
		case STATE_CIRCLE_RIGHT:
		{
			o->timer++;
			
			if (o->timer < 100)					mainangle += 1;
			else if (o->timer < 120)			mainangle += 2;
			else if (o->timer < o->timer2)		mainangle += 4;
			else if (o->timer < o->timer2 + 40)	mainangle += 2;
			else if (o->timer < o->timer2 + 60)	mainangle += 1;
			else
			{
				o->timer = 0;
				o->state = STATE_CIRCLE_LEFT;
				o->timer2 = random(400, 700);
			}
		}
		break;
		
		case STATE_CIRCLE_LEFT:
		{
			o->timer++;
			
			if (o->timer < 20)					mainangle -= 1;
			else if (o->timer < 60)				mainangle -= 2;
			else if (o->timer < o->timer2)		mainangle -= 4;
			else if (o->timer < o->timer2 + 40)	mainangle -= 2;
			else if (o->timer < o->timer2 + 60)	mainangle -= 1;
			else
			{
				if (o->hp < SISTERS_ATTACK2_HP)
				{
					o->state = STATE_MEGAFIRING;
					SetHeadStates(STATE_HEAD_MEGAFIRE);
					o->timer = 0;
				}
				else
				{
					o->state = STATE_CIRCLE_RIGHT;
					o->timer2 = random(400, 700);
					o->timer = 0;
				}
			}
		}
		break;
		
		// runs the circling while the heads are doing their low-hp "megafire" state.
		// first they stop completely, then spin around and around clockwise for a while.
		case STATE_MEGAFIRING:
		{
			if (++o->timer > 100)
			{
				o->state++;
				o->timer = 0;
			}
		}
		break;
		case STATE_MEGAFIRING+1:
		{
			o->timer++;
			
			if (o->timer < 100)		 mainangle += 1;
			else if (o->timer < 120) mainangle += 2;
			else if (o->timer < 500) mainangle += 4;
			else if (o->timer < 540) mainangle += 2;
			else if (o->timer < 560) mainangle += 1;
			else
			{
				o->state = STATE_CIRCLE_LEFT;
				SetHeadStates(STATE_HEAD_CLOSED);
				o->timer = 0;
			}
		}
		break;
		
		// defeated!! make big smoke and pause a second
		// this state is entered via a <BOA in the ondeath script.
		case STATE_DEFEATED:
		{
			SpawnScreenSmoke(40);
			
			for(i=0;i<NUM_SISTERS;i++)
			{
				head[i]->damage = 0;
				body[i]->damage = 0;
			}
			
			SetHeadStates(STATE_HEAD_DEFEATED);
			SetBodyStates(STATE_BODY_FACE_CENTER);
			
			o->state++;
			o->timer = 0;
		}
		case STATE_DEFEATED+1:
		{
			if (++o->timer > 100)
			{
				o->state = STATE_DEFEATED_CRASH;
				o->timer = 0;
			}
			
			SpawnScreenSmoke(1);
		}
		break;
		
		// the two dragons crash into each other
		case STATE_DEFEATED_CRASH:
		{
			mainangle += 4;
			
			if (o->xmark > 8) o->xmark--;
			if (o->ymark > 0) o->ymark--;
			
			if (++o->timer == 40)
				SetBodyStates(STATE_BODY_LOCK_DIR);
			
			if (o->ymark == 0)
			{
				if (hitdetect(head[0], head[1]) || \
					hitdetect(head[0], body[1]) || \
					hitdetect(head[1], body[0]))
				{
					starflash.Start(o->CenterX(), o->CenterY());
					NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPLOSION1);
					
					o->state = STATE_STARFLASH;
					o->timer = 0;
				}
				else
				{
					o->xmark -= 2;
					SetBodyStates(STATE_BODY_LOCK_DIR);	// lock direction facing
				}
			}
		}
		break;
		
		// big starflash after dragons hit each other
		case STATE_STARFLASH:
		{
			if (++o->timer > 30)
			{
				KillObjectsOfType(OBJ_DRAGON_ZOMBIE_SHOT);
				KillObjectsOfType(OBJ_SPIKE_SMALL);
				
				for(i=0;i<NUM_SISTERS;i++)
				{
					head[i]->Delete();
					body[i]->Delete();
				}
				
				main->Delete();
				main = NULL;
				
				return;
			}
		}
		break;
	}
	
	while(mainangle >= 256*4)  mainangle -= 256*4;
	while(mainangle <= -256*4) mainangle += 256*4;
	
	for(i=0;i<NUM_SISTERS;i++)
	{
		run_body(i);
		run_head(i);
	}
}

/*
void c------------------------------() {}
*/

void SistersBoss::run_body(int index)
{
	Object *o = body[index];
	
	uint8_t angle = (mainangle / 4);
	angle += (256 / NUM_SISTERS) * index;
	
	// main's xmark and ymark tell us how far from the center to circle
	int xoff = xinertia_from_angle(angle, main->xmark * CSFI);
	int yoff = yinertia_from_angle(angle, main->ymark * CSFI);
	
	// figure out where we are supposed to be
	int desired_x = (main->x + xoff) - (main->Width() / 2);
	int desired_y = (main->y + yoff) - (main->Height() / 2);
	
	// motion
	if (o->state == 0)
	{	// this places them offscreen before the fight
		o->x = desired_x;
		o->y = desired_y;
		o->state = STATE_BODY_NOMOVE;
	}
	else if (o->state != STATE_BODY_NOMOVE)
	{	// smooth interpolation, they stay in this state throughout the fight
		o->x += (desired_x - o->x) / 8;
		o->y += (desired_y - o->y) / 8;
	}
	
	// set direction facing
	if (o->state != STATE_BODY_LOCK_DIR)
	{
		if (o->state == STATE_BODY_FACE_CENTER)
		{
			if (o->CenterX() > main->CenterX()) o->dir = LEFT;
			else o->dir = RIGHT;
		}
		else
		{
			FACEPLAYER;
		}
	}
	
	ANIMATE(2, 0, 2);
}

void SistersBoss::run_head(int index)
{
	Object *o = head[index];
	
	// stay connected to body
	o->dir = body[index]->dir;
	o->x = body[index]->x + ((o->dir==RIGHT) ? (4 * CSFI) : -(4 * CSFI));
	o->y = body[index]->y - (4 * CSFI);
	
	// link hp to main object
	if (o->hp < 1000)
	{
		main->DealDamage(1000 - o->hp);
		o->hp = 1000;
	}
	
	// FSM
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->flags |= (FLAG_SHOOTABLE | FLAG_INVULNERABLE);
		}
		break;
		
		case STATE_HEAD_CLOSED:
		{
			o->frame = 0;
			o->timer = random(100, 200);
			o->state++;
		}
		case STATE_HEAD_CLOSED+1:
		{
			if (--o->timer <= 0)
			{
				o->state = STATE_HEAD_OPEN;
				o->timer = 0;
			}
		}
		break;
		
		// open mouth and prepare to fire
		case STATE_HEAD_OPEN:
		{
			o->timer = 0;
			o->timer2 = 0;
			o->state++;
		}
		case STATE_HEAD_OPEN+1:
		{
			o->timer++;
			if (o->timer == 3) o->frame = 1;	// mouth partially open--about to fire!!
			if (o->timer == 6) o->frame = 2;	// mouth fully open
			
			if (o->timer > 150)
			{	// begin firing if they haven't hit us by now
				o->state = STATE_HEAD_FIRE;
				o->timer = 0;
			}
			
			// need at least 2 hits to get her to close mouth
			if (o->shaketime) o->timer2++;
			if (o->timer2 > 10)
			{
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_HURT);
				SmokeClouds(o, 4, 2, 2);
				
				o->state = STATE_HEAD_BIT_TONGUE;
				o->timer = 0;
				o->frame = 3;
			}
		}
		break;
		
		// firing (normal)
		case STATE_HEAD_FIRE:
		{
			if ((++o->timer % 8) == 1)
			{
				EmFireAngledShot(o, OBJ_DRAGON_ZOMBIE_SHOT, 6, 0x200);
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SNAKE_FIRE);
			}
			
			if (o->timer > 50)
				o->state = STATE_HEAD_CLOSED;
		}
		break;
		
		// firing a WHOLE LOT during low-hp status
		case STATE_HEAD_MEGAFIRE:
		{
			o->state++;
			o->timer = 0;
			o->frame = 0;
		}
		case STATE_HEAD_MEGAFIRE+1:
		{
			o->timer++;
			if (o->timer == 3) o->frame = 1;
			if (o->timer == 6) o->frame = 2;
			
			if (o->timer > 20)
			{
				if ((o->timer % 32) == 1)
				{
					EmFireAngledShot(o, OBJ_DRAGON_ZOMBIE_SHOT, 6, 0x200);
					NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SNAKE_FIRE);
				}
			}
		}
		break;
		
		// mouth closed bit tongue after got shot
		case STATE_HEAD_BIT_TONGUE:
		{
			if (++o->timer > 100)
			{
				o->state = STATE_HEAD_CLOSED;
				o->timer = 0;
			}
		}
		break;
		
		case STATE_HEAD_DEFEATED:
			o->frame = 4;	// eyes closed
		break;
	}
	
	head_set_bbox(index);
}

// select an appropriate bbox for the current frame and hack it onto our sprite.
// I did this because the heads move around quite a bit in the different frames.
void SistersBoss::head_set_bbox(int index)
{
	SIFSprite *sprite = &sprites[SPR_SISTERS_HEAD_1 + index];
	Object *o = head[index];
	int frame = o->frame;
	
	if (o->dir == LEFT)
	{	// use mirror image if dragon facing left
		int wd = (head_bboxes[frame].x2 - head_bboxes[frame].x1);
		
		sprite->bbox.x1 = sprite->w - head_bboxes[frame].x1 - wd;
		sprite->bbox.x2 = sprite->bbox.x1 + (wd - 1);
	}
	else
	{
		sprite->bbox.x1 = head_bboxes[frame].x1;
		sprite->bbox.x2 = head_bboxes[frame].x2;
	}
	
	sprite->bbox.y1 = head_bboxes[frame].y1;
	sprite->bbox.y2 = head_bboxes[frame].y2;
	
	o->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE);
	o->flags |= head_bboxes[frame].flags;
}

/*
void c------------------------------() {}
*/

void SistersBoss::SpawnScreenSmoke(int count)
{
	for(int i=0;i<count;i++)
	{
		int x = random(1*TILE_W, 20*TILE_W) * CSFI;
		int y = random(2*TILE_H, 14*TILE_H) * CSFI;
		SmokePuff(x, y);
	}
}

/*
void c------------------------------() {}
*/

void SistersBoss::SetHeadStates(int newstate)
{
	for(int i=0;i<NUM_SISTERS;i++)
		head[i]->state = newstate;
}

void SistersBoss::SetBodyStates(int newstate)
{
	for(int i=0;i<NUM_SISTERS;i++)
		body[i]->state = newstate;
}
