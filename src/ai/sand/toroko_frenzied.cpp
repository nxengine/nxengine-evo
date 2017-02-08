#include "toroko_frenzied.h"

#include "../stdai.h"
#include "../ai.h"

#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_TOROKO_FRENZIED, ai_toroko_frenzied);
	
	ONTICK(OBJ_TOROKO_BLOCK, ai_toroko_block);
	AFTERMOVE(OBJ_TOROKO_BLOCK, aftermove_toroko_block);
	
	ONTICK(OBJ_TOROKO_FLOWER, ai_toroko_flower);
}

/*
void c------------------------------() {}
*/

void ai_toroko_frenzied(Object *o)
{
Object *block = o->linkedobject;

#define SPAWNBLOCK	\
{					\
	block = CreateObject(0, 0, OBJ_TOROKO_BLOCK);	\
	block->PushBehind(o);						\
	block->linkedobject = o;					\
	o->linkedobject = block;				\
	block->flags &= ~FLAG_INVULNERABLE;		\
}
#define THROWBLOCK	\
{			\
	block->x += (16 * CSFI) * ((o->dir==RIGHT) ? 1 : -1);	\
	block->y += (9 * CSFI);	\
	block->flags |= FLAG_INVULNERABLE;		\
	ThrowObjectAtPlayer(block, 1, 0x900);		\
	o->linkedobject = block->linkedobject = NULL;	\
	sound(SND_EM_FIRE);		\
}

#define HOLDBRICKTIME		30

	switch(o->state)
	{
		case 0:
			o->state = 1;
			o->frame = 9;
			o->flags &= ~(FLAG_SCRIPTONACTIVATE | FLAG_SHOOTABLE | FLAG_IGNORE_SOLID);
		case 1:		// wait a sec before morphing
			if (++o->timer > 50)
			{
				o->timer = 0;
				o->state = 2;
				o->frame = 8;
			}
		break;
		
		case 2:		// morph into big toroko
			ANIMATE(0, 9, 10);
			if (++o->timer > 50)
			{
				o->state = 3;
				o->timer = 0;
				o->flags |= FLAG_SHOOTABLE;
			}
		break;
		
		case 3:		// rest a moment, then jump
			o->frame = 1;
			if (++o->timer > 5)
			{
				o->state = 10;
				o->flags |= FLAG_SHOOTABLE;
			}
		break;
		
		case 10:	// wait a moment then ATTACK!!
			o->state = 11;
			o->frame = 0;
			o->animtimer = 0;
			o->timer = random(20, 130);
			o->xinertia = 0;
		case 11:
			FACEPLAYER;
			ANIMATE(4, 0, 1);
			///FIXME:: I think Toroko does not like fireball either?
			if (o->frame==0 && (sound_is_playing(SND_MISSILE_HIT) /*|| sound_is_playing(0)*/))
			{
				o->state = 20;
			}
			
			if (!o->timer)
			{
				o->state = (random(0, 1) ? 20 : 50);
			}
			else o->timer--;
		break;
		
		case 20:	// init for a jump
			o->state = 21;
			o->frame = 2;
			o->timer = 0;
		case 21:	// preparing to jump
			if (++o->timer > 10)
			{
				o->state = 22;
				o->timer = 0;
				o->frame = 3;
				o->yinertia = -0x5ff;
				XMOVE(0x200);
			}
		break;
		case 22:	// jumping up
			if (++o->timer > 10) { o->state = 23; o->timer = 0; o->frame = 6; SPAWNBLOCK; }
		break;
		case 23:	// preparing to throw block
			if (++o->timer > HOLDBRICKTIME) { o->state = 24; o->timer = 0; o->frame = 7; THROWBLOCK; }
			FACEPLAYER;
		break;
		case 24:	// threw block
			if (++o->timer > 3) { o->state = 25; o->frame = 3; }
		break;
		case 25:	// coming back down
			if (o->blockd)
			{
				o->state = 26;
				o->timer = 0;
				o->frame = 2;
				sound(SND_QUAKE);
				game.quaketime = 20;
			}
		break;
		case 26:	// landed
			o->xinertia *= 8;
			o->xinertia /= 9;
			if (++o->timer > 20)
			{
				o->state = 10;
				o->frame = 0;
			}
		break;
		
		case 50:	// throw a block (standing on ground)
			o->state = 51;
			o->timer = 0;
			o->frame = 4;
			SPAWNBLOCK;
		case 51:
			if (++o->timer > HOLDBRICKTIME)
			{
				o->state = 52;
				o->timer = 0;
				o->frame = 5;
				THROWBLOCK;
			}
			FACEPLAYER;
		break;
		case 52:
			if (++o->timer > 3) { o->state = 10; o->frame = 0; }
		break;
		
		case 100:		// defeated (set by Script On Death)
			o->frame = 3;
			o->state = 101;
			o->flags &= ~FLAG_SHOOTABLE;
			SmokeClouds(o, 8, 8, 8);
		case 101:		// wait till fall to ground
			if (o->blockd)
			{
				o->state = 102;
				o->frame = 2;
				o->timer = 0;
				sound(SND_QUAKE);
				game.quaketime = 20;
			}
		break;
		case 102:		// crouching & smoking
			o->xinertia *= 8; o->xinertia /= 9;
			if (++o->timer > 50) { o->state = 103; o->timer = 0; o->frame = 10; }
		break;
		case 103:		// looks pained
			if (++o->timer > 50) { o->state = 104; o->timer = 0; o->frame = 9; }
		break;
		case 104:		// morphing back into normal toroko
			o->frame = (o->frame==9) ? 10:9;
			if (++o->timer > 100) { o->state = 105; o->timer = 0; o->frame = 9; }
		break;
		case 105:		// back to normal
			if (++o->timer > 50) { o->state = 106; o->animtimer = 0; o->frame = 11; }
		break;
		case 106:		// red goes out of her eyes, she falls down
			if (++o->animtimer > 50)
			{	// collapse
				o->animtimer = 0;
				if (++o->frame > 12) o->frame = 12;
			}
		break;
		
		case 140:		// disappear
			o->state = 141;
			o->timer = 0;
			o->frame = 12;
			sound(SND_TELEPORT);
		case 141:
			o->invisible ^= 1;
			if (++o->timer > 100)
			{
				// normal SmokeClouds() doesn't spawn in the right place because the sprite
				// is still sized for big toroko, so I made an Action Point for this frame.
				SmokePuff(o->ActionPointX(), o->ActionPointY());
				SmokePuff(o->ActionPointX(), o->ActionPointY());
				SmokePuff(o->ActionPointX(), o->ActionPointY());
				SmokePuff(o->ActionPointX(), o->ActionPointY());
				o->Delete();
			}
		break;
	}
	
	if (o->state > 100 && o->state <= 105)
	{
		if ((o->timer % 9)==1)
		{
			SmokeClouds(o, 1, 8, 8);
		}
	}
	
	o->yinertia += 0x20;
	LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/


// the blocks Frenzied Toroko throws
void ai_toroko_block(Object *o)
{
int hit;
	o->frame ^= 1;
	
	// if he's still holding us, just let the _am routine stick us to his action point
	if (o->linkedobject) return;
	
	hit = 0;
	if (o->xinertia < 0) hit = o->blockl;
	else if (o->xinertia > 0) hit = o->blockr;
	
	if (o->yinertia < 0) hit |= o->blocku;
	else if (o->yinertia > 0) hit |= o->blockd;
	
	if (hit)
	{
		SmokeClouds(o, 6, 8, 8);
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		sound(SND_BLOCK_DESTROY);
		
		o->ChangeType(OBJ_TOROKO_FLOWER);
		o->frame = 0;
		o->state = 20;
		o->xinertia = 0;
		o->flags &= ~FLAG_INVULNERABLE;
		o->flags |= FLAG_SHOOTABLE;
		o->BringToFront();		// block was behind toro, but flowers are in front
	}
}

void aftermove_toroko_block(Object *o)
{
	if (o->linkedobject)
		aftermove_StickToLinkedActionPoint(o);
}

void ai_toroko_flower(Object *o)
{
	switch(o->state)
	{
		case 10:
			o->state = 11;
			o->timer = o->frame = 0;
		case 11:
			if (++o->timer > 30)
			{
				o->state = 12;
				o->frame = 1;
				o->animtimer = 0;
			}
		break;
		case 12:
			ANIMATE_FWD(8);
			if (o->frame == 3)
			{
				o->state = 20;
				o->yinertia = -0x200;
				o->xinertia = (o->x > player->x) ? -0x200 : 0x200;
			}
		break;
		
		case 20:	// falling/jumping
			if (o->yinertia > -0x80) o->frame = 4; else o->frame = 3;
			if (o->blockd)
			{
				o->frame = 2;
				o->state = 21;
				o->timer = o->xinertia = 0;
				sound(SND_THUD);
			}
		break;
		case 21:
			if (++o->timer > 10) { o->state = 10; o->frame = 0; }
		break;
	}
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}




