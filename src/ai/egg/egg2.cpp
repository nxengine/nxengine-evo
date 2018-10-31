#include "egg2.h"

#include "egg.h"
#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../weed/weed.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../common/misc.h"

#include "../../player.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CRITTER_HOPPING_AQUA, ai_critter);
	
	ONTICK(OBJ_BEETLE_FREEFLY_2, ai_beetle_freefly);
	ONTICK(OBJ_GIANT_BEETLE_2, ai_giant_beetle);
	
	ONTICK(OBJ_DRAGON_ZOMBIE, ai_dragon_zombie);
	ONTICK(OBJ_DRAGON_ZOMBIE_SHOT, ai_generic_angled_shot);
	
	ONTICK(OBJ_FALLING_SPIKE_SMALL, ai_falling_spike_small);
	ONTICK(OBJ_FALLING_SPIKE_LARGE, ai_falling_spike_large);
	
	ONTICK(OBJ_COUNTER_BOMB, ai_counter_bomb);
	ONTICK(OBJ_COUNTER_BOMB_NUMBER, ai_counter_bomb_number);
}

/*
void c------------------------------() {}
*/

void ai_dragon_zombie(Object *o)
{
	if (o->hp < 950 && o->state < 50)
	{
		NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BIG_CRASH);
		SmokeClouds(o, 8, 4, 4);
		o->SpawnPowerups();
		
		o->flags &= ~FLAG_SHOOTABLE;
		o->damage = 0;
		
		o->frame = 5;	// dead
		o->state = 50;	// dead
	}
	
	switch(o->state)
	{
		case 0:
		case 1:		// ready
		{
			ANIMATE(30, 0, 1);
			
			if (--o->timer < 0)		// can fire again yet?
			{
				if (pdistlx(112 * CSFI))	// player in range?
				{
					o->state = 2;
					o->timer = 0;
				}
			}
		}
		break;
		
		case 2:		// flashing, prepare to fire
		{
			FACEPLAYER;
			
			o->timer++;
			o->frame = (o->timer & 2) ? 2 : 3;
			
			if (o->timer > 30)
				o->state = 3;
		}
		break;
		
		case 3:
		{
			o->state = 4;
			o->timer = 0;
			o->frame = 4;
			
			// save point we'll fire at--these enemies don't update
			// the position of their target for each shot
			o->xmark = player->x;
			o->ymark = player->y;
		}
		case 4:
		{
			o->timer++;
			
			if (o->timer < 40 && (o->timer % 8) == 1)
			{
				Object *fire = SpawnObjectAtActionPoint(o, OBJ_DRAGON_ZOMBIE_SHOT);
				ThrowObject(fire, o->xmark, o->ymark, 6, 0x600);
				
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SNAKE_FIRE);
			}
			
			if (o->timer > 60)
			{
				o->state = 1;
				o->frame = 0;
				o->timer = random(100, 200);	// random time till can fire again
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_falling_spike_small(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->xmark = o->x;
			
			if (pdistlx(12 * CSFI))
				o->state = 1;
		}
		break;
		
		case 1:		// shaking
		{
			if (++o->animtimer >= 12)
				o->animtimer = 0;
			
			o->x = o->xmark;
			if (o->animtimer >= 6) o->x += (1 * CSFI);
			
			if (++o->timer > 30)
			{
				o->state = 2;	// fall
				o->frame = 1;	// slightly brighter frame at top
			}
		}
		break;
		
		case 2:		// falling
		{
			o->yinertia += 0x20;
			LIMITY(0xC00);
			
			if (o->blockd)
			{
				if (!player->inputs_locked)	// no sound in ending cutscene
					NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
				
				SmokeClouds(o, 4, 2, 2);
				effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
				o->Delete();
			}
		}
		break;
	}
}


void ai_falling_spike_large(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->xmark = o->x;
			
			if (pdistlx(12 * CSFI))
				o->state = 1;
		}
		break;
		
		case 1:		// shaking
		{
			if (++o->animtimer >= 12)
				o->animtimer = 0;
			
			o->x = o->xmark;
			if (o->animtimer >= 6)	// scuttle:: big spikes shake in the other direction
				o->x -= (1 * CSFI);
			
			if (++o->timer > 30)
			{
				o->state = 2;	// fall
				o->frame = 1;	// slightly brighter frame at top
			}
		}
		break;
		
		case 2:		// falling
		{
			o->yinertia += 0x20;
			LIMITY(0xC00);
			
			if (o->Bottom() < player->Bottom())
			{	// could fall on player
				o->flags &= ~FLAG_SOLID_BRICK;
				o->damage = 127;	// ouch!
			}
			else
			{	// player could only touch side from this position
				o->flags |= FLAG_SOLID_BRICK;
				o->damage = 0;
			}
			
			// damage NPC's as well (it kills that one Dragon Zombie)
			Object *enemy;
			FOREACH_OBJECT(enemy)
			{
				if ((enemy->flags & FLAG_SHOOTABLE) && \
					o->Bottom() >= enemy->CenterY() && hitdetect(o, enemy))
				{
					if (!(enemy->flags & FLAG_INVULNERABLE))
						enemy->DealDamage(127);
				}
			}
			
			if (++o->timer > 8 && o->blockd)
			{
				o->flags |= FLAG_SOLID_BRICK;
				o->damage = 0;
				o->yinertia = 0;
				
				o->state = 3;	// fall complete
				o->timer = 0;
				
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
				SmokeClouds(o, 4, 2, 2);
				
				effect(o->CenterX(), o->y + (sprites[o->sprite].block_d[0].y * CSFI),
					EFFECT_STARSOLID);
			}
		}
		break;
		
		case 3:		// hit ground
		{
			if (++o->timer > 4)
			{	// make it destroyable
				o->flags |= FLAG_SHOOTABLE;
				o->flags &= ~FLAG_INVULNERABLE;
				o->state = 4;
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_counter_bomb(Object *o)
{
	/*debug("state: %d", o->state);
	debug("timer: %d", o->timer);
	debug("timer2: %d", o->timer2);*/
	
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->ymark = o->y;
			
			o->timer = random(0, 50);
			o->timer2 = 0;
		}
		case 1:
		{	// desync if multiple enemies
			if (--o->timer < 0)
			{
				o->timer = 0;
				o->state = 2;
				o->yinertia = 0x300;
			}
		}
		break;
		
		case 2:		// ready
		{
			if (pdistlx(80 * CSFI) || o->shaketime)
			{
				o->state = 3;
				o->timer = 0;
			}
		}
		break;
		
		case 3:		// counting down...
		{
			if (--o->timer < 0)
			{
				if (o->timer2 < 5)
				{
					Object *number = CreateObject(o->CenterX() + (8 * CSFI), \
												  o->y + (16 * CSFI), \
												  OBJ_COUNTER_BOMB_NUMBER);
					
					number->frame = o->timer2++;
					o->timer = 60;
				}
				else
				{
					// expand bounding box to cover explosion area
					o->x = o->CenterX();
					o->y = o->CenterY();
					o->invisible = true;
					o->sprite = SPR_BBOX_PUPPET_1;
					sprites[o->sprite].bbox.x1 = -128;
					sprites[o->sprite].bbox.y1 = -100;
					sprites[o->sprite].bbox.x2 = 128;
					sprites[o->sprite].bbox.y2 = 100;
					o->damage = 30;
					
					o->yinertia = 0;
					o->state = 4;
					
					// make kaboom
					NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPLOSION1);
					quake(20);
					SmokeXY(o->CenterX(), o->CenterY(), 100, 128, 100);
					
					return;
				}
			}
		}
		break;
		
		case 4:		// exploding (one frame only to give time for bbox to damage player)
			o->Delete();
			return;
	}
	
	ANIMATE(4, 0, 2);
	
	if (o->state == 2 || o->state == 3)
	{
		o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;
		LIMITY(0x100);
	}
}

void ai_counter_bomb_number(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
			o->state = 1;
		}
		case 1:
		{
			o->x += (1 * CSFI);
			if (++o->timer > 8)
			{
				o->state = 2;
				o->timer = 0;
			}
		}
		break;
		
		case 2:
		{
			if (++o->timer > 30)
				o->Delete();
		}
		break;
	}
}
