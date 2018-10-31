#include "maze.h"
#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../../game.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_BLOCK_MOVEH, ai_block_moveh);
	ONTICK(OBJ_BLOCK_MOVEV, ai_block_movev);
	
	ONTICK(OBJ_BOULDER, ai_boulder);
	
	GENERIC_NPC_NOFACEPLAYER(OBJ_GAUDI_SHOPKEEP);
}

/*
void c------------------------------() {}
*/

void ai_block_moveh(Object *o)
{
	int px = player->CenterX();
	int objx = o->CenterX();
	
	switch(o->state)
	{
		case 0:
			o->flags |= FLAG_SOLID_BRICK;
			o->smushdamage = 100;
			o->state = (o->dir == LEFT) ? 10:20;
		break;
		
		case 10:	// at right edge, ready to travel left
			if (((px > objx) && (px - objx) < 0x3200) || \
				((px < objx) && (objx - px) < 0x32000))
			{
				if (pdistly(0x3200))
				{
					o->state = 30;
					o->timer = 0;
				}
			}
		break;
		
		case 20:	// at left edge, ready to travel right
			if (((px > objx) && (px - objx) < 0x32000) || \
				((px < objx) && (objx - px) < 0x3200))
			{
				if (pdistly(0x3200))
				{
					o->state = 30;
					o->timer = 0;
				}
			}
		break;
		
		case 30:	// traveling
		{
			XACCEL(0x20);
			LIMITX(0x200);
			
			// hit edge
			if ((o->dir == RIGHT && o->blockr) || (o->dir == LEFT && o->blockl))
			{
				SmokeSide(o, 4, o->dir);
				quake(10);
				
				o->xinertia = 0;
				o->dir ^= 1;
				o->state = (o->dir==LEFT) ? 10 : 20;
			}
			
			if ((++o->timer % 10) == 6)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_MOVE);
		}
		break;
	}
}

void ai_block_movev(Object *o)
{
	int py = player->CenterY();
	int objy = o->CenterY();
	
	switch(o->state)
	{
		case 0:
			o->flags |= FLAG_SOLID_BRICK;
			o->smushdamage = 100;
			o->dir = (o->dir == LEFT) ? UP : DOWN;
			o->state = (o->dir == DOWN) ? 10 : 20;
		break;
		
		case 10:	// at top edge, ready to travel down
			if (((py > objy) && (py - objy) < 0x32000) || \
				((py < objy) && (objy - py) < 0x3200))
			{
				if (pdistlx(0x3200))
				{
					o->state = 30;
					o->timer = 0;
				}
			}
		break;
		
		case 20:	// at bottom edge, ready to travel up
			if (((py > objy) && (py - objy) < 0x3200) || \
				((py < objy) && (objy - py) < 0x32000))
			{
				if (pdistlx(0x3200))
				{
					o->state = 30;
					o->timer = 0;
				}
			}
		break;
		
		case 30:	// traveling
		{
			YACCEL(0x20);
			LIMITY(0x200);
			
			// hit edge
			if ((o->dir == DOWN && o->blockd) || (o->dir == UP && o->blocku))
			{
				SmokeSide(o, 4, o->dir);
				quake(10);
				
				o->yinertia = 0;
				o->dir ^= 1;
				o->state = (o->dir==DOWN) ? 10 : 20;
			}
			
			if ((++o->timer % 10) == 6)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_MOVE);
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_boulder(Object *o)
{
	switch(o->state)
	{
		// shaking
		case 10:
		{
			o->state = 11;
			o->timer = 0;
			o->xmark = o->x;
		}
		case 11:
		{
			if ((++o->timer % 3) != 0)
				o->x = o->xmark + (1 * CSFI);
			else
				o->x = o->xmark;
		}
		break;
		
		// thrown away by Balrog
		case 20:
		{
			o->yinertia = -0x400;
			o->xinertia = 0x100;
			NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);
			
			o->state = 21;
			o->timer = 0;
		}
		case 21:
		{
			o->yinertia += 0x10;
			
			if (o->blockd && o->yinertia >= 0)
			{
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPLOSION1);
				game.quaketime = 40;
				
				o->xinertia = 0;
				o->yinertia = 0;
				o->state = 0;
			}
		}
		break;
	}
}
