#include "doctor_common.h"

#include "../stdai.h"
#include "../../ObjManager.h"
#include "../../sound/SoundManager.h"
#include "../../graphics/sprites.h"
#include "../../game.h"

int crystal_xmark, crystal_ymark;
bool crystal_tofront;


Object *dr_create_red_crystal(int x, int y)
{
Object *upper_xtal;

	// we have to create these with the one which will bring itself to front
	// already having higher zorder (create it 2nd), because the code to
	// run aftermoves currently doesn't create a list first and so running
	// BringToFront from an aftermove can cause it's AI to be executed twice.
				  CreateObject(x, y, OBJ_RED_CRYSTAL)->dir = RIGHT;
	(upper_xtal = CreateObject(x, y, OBJ_RED_CRYSTAL))->dir = LEFT;
	
	return upper_xtal;
}

/*
void c------------------------------() {}
*/

void dr_tp_out_init(Object *o)
{
	o->ResetClip();
	o->clip_enable = true;
	
	NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
	o->shaketime = 0;	// show any waiting damage numbers NOW
}

bool dr_tp_out(Object *o)
{
	o->clipy1 += 2;
	o->clipy2 -= 2;
	
	if (o->clipy1 >= o->clipy2)
	{
		o->clip_enable = false;
		o->invisible = true;
		
		return true;
	}
	
	return false;
}

/*
void c------------------------------() {}
*/

void dr_tp_in_init(Object *o)
{
	o->clipy1 = o->clipy2 = (sprites[o->sprite].h / 2);
	o->clip_enable = true;
	o->invisible = false;
}

bool dr_tp_in(Object *o)
{
	o->clipy1 -= 2;
	o->clipy2 += 2;
	
	if (o->clipy1 <= 0 || o->clipy2 >= sprites[o->sprite].h)
	{
		o->clip_enable = false;
		o->ResetClip();
		return true;
	}
	
	return false;
}
