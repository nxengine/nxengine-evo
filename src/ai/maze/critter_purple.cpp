#include "critter_purple.h"

#include "../stdai.h"
#include "../ai.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../game.h"
#include "../../player.h"

#define STATE_IDLE			1		// waiting for player to come near
#define STATE_PREPARE_JUMP	2		// on ground, prepare to jump
#define STATE_JUMP			3		// jumping up
#define STATE_HOVER			4		// hovering and firing
#define STATE_END_JUMP		5		// falling back down

#define CRITTER_DAMAGE		2		// normal damage
#define CRITTER_FALL_DAMAGE	3		// damage if it falls on you


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CRITTER_SHOOTING_PURPLE, ai_critter_shooting_purple);
	ONTICK(OBJ_CRITTER_SHOT, ai_generic_angled_shot);
}

/*
void c------------------------------() {}
*/

// used only for purple ones in maze
void ai_critter_shooting_purple(Object *o)
{

	switch(o->state)
	{
		case 0:
			o->state = STATE_IDLE;
			o->damage = CRITTER_DAMAGE;
		case STATE_IDLE:
		{
			o->frame = 0;		// assume not at attention
			if (o->timer >= 8)
			{
				if (pdistlx(96 * CSFI) && pdistly2(96 * CSFI, 32 * CSFI))
				{
					FACEPLAYER;
					
					// close enough to attack?
					if (pdistlx(48 * CSFI))
					{
						o->state = STATE_PREPARE_JUMP;
						o->frame = 0;
						o->timer = 0;
					}
					else
					{	// no, but stand at "attention"
						o->frame = 1;
					}
				}
			}
			else
			{
				o->timer++;
			}
			
			// also attack if shot
			if (o->shaketime)
			{
				o->state = STATE_PREPARE_JUMP;
				o->frame = 0;
				o->timer = 0;
			}
		}
		break;
		
		case STATE_PREPARE_JUMP:
		{
			o->frame = 1;
			if (++o->timer > 8)
			{
				FACEPLAYER;
				
				o->state = STATE_JUMP;
				o->timer = 0;
				o->frame = 2;
				
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);
				o->yinertia = -0x5ff;
			}
		}
		break;
		
		case STATE_JUMP:
		{
			if (o->yinertia > 0x100 || \
				(o->blockd && ++o->timer > 16))		// failsafe
			{
				o->ymark = o->y;
				
				o->state = STATE_HOVER;
				o->frame = 3;
				o->timer = 0;
				
				o->CurlyTargetHere(60, 100);
			}
		}
		break;
		
		case STATE_HOVER:
		{	// sinusoidal hover
			o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;
			LIMITY(0x200);
			
			FACEPLAYER;
			ANIMATE(0, 3, 5);
			
			// time to end flight?
			if (++o->timer > 60 || o->blocku)
			{
				o->damage = CRITTER_FALL_DAMAGE;
				o->state = STATE_END_JUMP;
				o->frame = 2;
				break;
			}
			
			if ((o->timer % 4) == 1)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CRITTER_FLY);
			
			if ((o->timer % 30) == 6)
			{
				EmFireAngledShot(o, OBJ_CRITTER_SHOT, 6, 0x600);
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
			}
			
			if (o->blockd)
				o->yinertia = -0x200;
		}
		break;
		
		case STATE_END_JUMP:
		{
			if (o->blockd)
			{
				o->damage = 2;
				o->xinertia = 0;
				o->timer = 0;
				o->frame = 0;
				o->state = 0;
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
			}
		}
		break;
	}
	
	if (o->state != STATE_HOVER)
	{
		o->yinertia += 0x20;
		LIMITY(0x5ff);
	}
}
