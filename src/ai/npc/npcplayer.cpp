#include "npcplayer.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../tsc.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../map.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


/*
	This is an object which looks exactly like the player,
	but is controlled as if he is an NPC. Used during cutscenes.
*/

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_NPC_PLAYER, ai_npc_player);
	ONTICK(OBJ_PTELIN, ai_ptelin);
	ONTICK(OBJ_PTELOUT, ai_ptelout);
}

/*
void c------------------------------() {}
*/

void ai_npc_player(Object *o)
{
static const int pwalkanimframes[] = { 0, 1, 0, 2 };
#define NUM_PWALK_FRAMES		4
	o->sprite = (player->equipmask & EQUIP_MIMIGA_MASK) ? \
	                                        SPR_MYCHAR_MIMIGA : SPR_MYCHAR;
	switch(o->state)
	{
		case 0:
		{
			o->frame = 0;
			o->xinertia = 0;
			
			// used during Hermit Gunsmith scene when he is awake
			if (o->dirparam >= 10)
			{
				o->x = player->x;
				o->y = player->y;
				o->dir = CVTDir(o->dirparam - 10);
				o->dirparam = 0;
			}
		}
		break;
		
		case 2:		// looking up
		{
			o->frame = 3;
		}
		break;
		
		case 10:	// he gets flattened
		{
			sound(SND_LITTLE_CRASH);
			SmokeClouds(o, 6, 8, 8);
			o->state++;
		}
		case 11:
		{
			o->frame = 9;
		}
		break;
		
		case 20:	// he teleports away
		{
			if (DoTeleportOut(o, 2))
				o->Delete();
		}
		break;
		
		case 50:	// walking
		{
			// z-order tweaking for oside bad-ending
			Object *dragon = Objects::FindByType(OBJ_SKY_DRAGON);
			if (dragon) o->PushBehind(dragon);
			
			o->state = 51;
			o->animframe = 0;
			o->animtimer = 0;
		}
		case 51:
		{
			o->animate_seq(4, pwalkanimframes, NUM_PWALK_FRAMES);
			XMOVE(0x200);
		}
		break;
		
		// falling, upside-down (from good ending; Fall stage)
		case 60:
		{
			o->state = 61;
			o->frame = 10;
			o->xmark = o->x;
			o->ymark = o->y;
		}
		case 61:
		{
			o->ymark += 0x100;
			o->x = o->xmark + (random(-1, 1) * CSFI);
			o->y = o->ymark + (random(-1, 1) * CSFI);
		}
		break;
		
		case 80:	// face away
			o->frame = 11;
		break;
		
		// walking in place during credits
		case 99:
		case 100:
		{
			o->state = 101;
			o->frame = 1;
			o->animtimer = 0;
			
			if ((player->equipmask & EQUIP_MIMIGA_MASK) || game.flags[1020])
				o->sprite = SPR_MYCHAR_MIMIGA;
		}
		case 101:	// falling a short dist
		case 102:	// walk in place
		{
			if (!o->blockd)
			{
				o->yinertia += 0x40;
				LIMITY(0x5ff);
			}
			else
			{
				o->yinertia = 0;
				o->animate_seq(8, pwalkanimframes, NUM_PWALK_FRAMES);
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

// player (teleporting in)
void ai_ptelin(Object *o)
{
	o->sprite = (player->equipmask & EQUIP_MIMIGA_MASK) ? \
	                                        SPR_MYCHAR_MIMIGA : SPR_MYCHAR;

	switch(o->state)
	{
		case 0:
		{
			o->flags &= ~FLAG_IGNORE_SOLID;
			o->frame = 0;
			o->timer = 0;
			o->x += (TILE_W * CSFI);
			o->y += (TILE_H / 2) * CSFI;
			o->state++;
			
			// note, it looks sort of like we might be supposed to face left when
			// appearing at the Labyrinth teleporter as well, but the original engine
			// does not do this, so I'm following what it does.
			if (game.curmap == STAGE_SAND)
				o->dir = LEFT;	// for Sand Zone, hackety
		}
		case 1:
		{
			if (DoTeleportIn(o, 2))
			{
				o->timer = 0;
				o->state = 2;
			}
		}
		break;
		
		case 2:
		{
			if (++o->timer > 20)
			{
				o->yinertia += 0x40;
				o->frame = 1;
				o->state = 3;
			}
		}
		break;
		
		case 3:
		{
			o->yinertia += 0x40;
			if (o->blockd)
			{
				o->frame = 0;
				o->state = 4;
			}
		}
	}
}


// player (teleporting out)
void ai_ptelout(Object *o)
{
	o->sprite = (player->equipmask & EQUIP_MIMIGA_MASK) ? \
	                                        SPR_MYCHAR_MIMIGA : SPR_MYCHAR;
	switch(o->state)
	{
		case 0:
		{
			o->y -= (TILE_H * CSFI);
			o->ymark = o->y - (8 * CSFI);
			
			o->frame = 0;
			o->timer = 0;
			o->state = 1;
		}
		break;
		
		case 1:
		{
			if (++o->timer > 20)
			{
				o->state = 2;
				o->frame = 1;
				o->timer = 0;
				o->yinertia = -0x2FF;
			}
		}
		break;
		
		case 2:
		{
			if (o->yinertia >= 0 && o->y >= o->ymark)
			{
				o->y = o->ymark;
				o->yinertia = 0;
				
				o->state = 3;
				o->frame = 0;
				o->timer = 0;
			}
		}
		break;
		
		case 3:
		{
			if (++o->timer > 40)
			{
				o->state = 4;
				o->timer = 0;
			}
		}
		break;
		
		case 4:
		{
			if (DoTeleportOut(o, 2))
				o->Delete();
		}
		break;
	}
	
	if (o->state < 3)
		o->yinertia += 50;
	
	LIMITY(0x5ff);
}




















