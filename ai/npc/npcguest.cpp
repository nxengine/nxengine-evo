#include "npcguest.h"

#include "../stdai.h"
#include "../ai.h"

#include "../../game.h"
#include "../../player.h"
//#include "../../map.h"
#include "../../graphics/tileset.h"


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_MAHIN, ai_npc_mahin);
	
	ONSPAWN(OBJ_YAMASHITA_FLOWERS, onspawn_set_frame_from_id2);
	ONTICK(OBJ_YAMASHITA_PAVILION, ai_yamashita_pavilion);
	
	ONTICK(OBJ_CHTHULU, ai_chthulu);
	
	GENERIC_NPC_NOFACEPLAYER(OBJ_DR_GERO);
	GENERIC_NPC_NOFACEPLAYER(OBJ_NURSE_HASUMI);
}

/*
void c------------------------------() {}
*/

void ai_chthulu(Object *o)
{
	if (!o->state)
	{
		o->SnapToGround();
		o->state = 1;
	}
	
	// open eyes when player comes near
	o->frame = (pdistlx((48<<CSF)) && pdistly2((48<<CSF), (16<<CSF))) ? 1 : 0;
}


// fat mimiga from village
void ai_npc_mahin(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->SnapToGround();
			o->state = 1;
			o->frame = 2;
		
		case 1:	// facing away
		break;
		
		case 2:	// talking to player
			o->frame = 0;
			randblink(o);
		break;
	}
}

/*
void c------------------------------() {}
*/

// Sandaime's Pavilion from Yamashita Farm
void ai_yamashita_pavilion(Object *o)
{
	if (!o->state)
	{
		if (o->dir == LEFT)
		{	// Sandaime present
			o->state = 1;
		}
		else
		{	// Sandaime gone
			o->x -= (TILE_W << CSF);
			o->state = 2;
			o->frame = 2;
		}
	}
	
	if (o->state == 1)
	{
		o->frame = 0;
		randblink(o);
	}
}

















