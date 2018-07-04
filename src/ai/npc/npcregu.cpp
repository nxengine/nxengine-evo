#include "npcregu.h"

#include "../stdai.h"
#include "../ai.h"
#include "../final_battle/doctor_common.h" //for Sue redcrystal
#include "../final_battle/doctor.h"
#include "../sand/puppy.h" // for ZZZZ

#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../map.h"
#include "../../sound/sound.h"
#include "../../common/stat.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"



INITFUNC(AIRoutines)
{
	ONTICK(OBJ_KAZUMA_AT_COMPUTER, ai_npc_at_computer);
	ONTICK(OBJ_SUE_AT_COMPUTER, ai_npc_at_computer);
	
	ONTICK(OBJ_JENKA, ai_jenka);
	ONTICK(OBJ_BLUE_ROBOT, ai_blue_robot);
	
	ONTICK(OBJ_DOCTOR, ai_doctor);
	
	ONTICK(OBJ_TOROKO, ai_toroko);
	ONTICK(OBJ_TOROKO_TELEPORT_IN, ai_toroko_teleport_in);
	
	ONTICK(OBJ_SUE, ai_npc_sue);
	AFTERMOVE(OBJ_SUE, aftermove_npc_sue);
	ONSPAWN(OBJ_SUE, onspawn_npc_sue);
	ONTICK(OBJ_SUE_TELEPORT_IN, ai_sue_teleport_in);
	
	ONTICK(OBJ_KING, ai_king);
	AFTERMOVE(OBJ_KINGS_SWORD, aftermove_StickToLinkedActionPoint);
	
	ONTICK(OBJ_KANPACHI_FISHING, ai_kanpachi_fishing);
	
	ONTICK(OBJ_PROFESSOR_BOOSTER, ai_professor_booster);
	ONTICK(OBJ_BOOSTER_FALLING, ai_booster_falling);
	
	GENERIC_NPC(OBJ_SANTA);
	GENERIC_NPC(OBJ_CHACO);
	GENERIC_NPC(OBJ_JACK);
	GENERIC_NPC_NOFACEPLAYER(OBJ_KAZUMA);
}

/*
void c------------------------------() {}
*/

void ai_jenka(Object *o)
{
	switch(o->state)
	{
		case 0: o->state = 1; o->frame = 0;
		case 1: if (random(0, 120) == 10) { o->state = 2; o->timer = 0; o->frame = 1; } break;
		case 2: if (++o->timer > 8) { o->state = 1; o->frame = 0; } break;
	}
}


void ai_doctor(Object *o)
{
	switch(o->state)
	{
		case 10:	// he chuckles
			o->state = 11;
			o->timer2 = 0;
			o->frame = 1;
			o->animtimer = 0;
		case 11:
			ANIMATE(6, 0, 1);
			if (++o->timer2 > 8*6) { o->frame = 0; o->state = 1; }
		break;
		
		case 20:	// he rises up and hovers
		{
			o->state = 21;
			o->timer = 0;
			o->frame = 2;
			o->ymark = o->y - (32 * CSFI);
		}
		case 21:
		{
			o->yinertia += (o->y > o->ymark) ? -0x20 : 0x20;
			LIMITY(0x200);
		}
		break;
		
		case 30:	// he teleports away
		{
			o->timer = 0;
			o->frame = 2;
			o->yinertia = 0;
			o->state++;
		}
		case 31:
		{
			if (DoTeleportOut(o, 1))
				o->Delete();
		}
		break;
		
		case 40:	// he teleports in and hovers
		{
			o->timer = 0;
			o->state = 41;
			o->frame = 2;
		}
		case 41:
		{
			if (DoTeleportIn(o, 1))
			{
				o->state = 20;
				o->yinertia = -0x200;
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void ai_toroko(Object *o)
{
	switch(o->state)
	{
		case 0:		// stand and blink
			o->frame = 0;
			o->xinertia = 0;
			randblink(o, 1, 4);
		break;
		
		case 3:		// run away!!
			o->state = 4;
			o->frame = 1;
			o->animtimer = 0;
		case 4:
			ANIMATE(2, 1, 4);
			
			if (o->blockl) { o->dir = RIGHT; o->xinertia = 0x200; }
			if (o->blockr) { o->dir = LEFT; o->xinertia = -0x200; }
			
			XMOVE(0x400);
		break;
		
		case 6:		// hop and run away!!
			o->state = 7;
			o->frame = 1;
			o->animtimer = 0;
			o->yinertia = -0x400;
			o->toro.left_ground = false;
		case 7:
			ANIMATE(2, 1, 4);
			XMOVE(0x100);
			
			if (!o->toro.left_ground)
			{
				if (!o->blockd)
					o->toro.left_ground = true;
			}
			else
			{
				if (o->blockd)
				{
					o->toro.left_ground = false;
					o->state = 3;
				}
			}
		break;
		
		// small hop straight up/down from Balrog
		// shaking the ground, used in Shack
		case 8:
			o->frame = 1;
			o->timer = 0;
			o->state = 9;
			o->yinertia = -0x200;
			o->toro.left_ground = false;
		case 9:
		{
			if (!o->toro.left_ground)
			{
				if (!o->blockd)
					o->toro.left_ground = true;
			}
			else
			{
				if (o->blockd)
				{
					o->toro.left_ground = false;
					o->state = 0;
				}
			}
		}
		break;
		
		case 10:	// eeks and falls down
			o->state = 11;
			o->frame = 5;
			o->yinertia = -(2 * CSFI);
			sound(SND_ENEMY_SQUEAK);
			XMOVE(0x100);
		break;
		case 11:	// falling down
			if (o->blockd)
			{
				o->state = 12;
				o->frame = 6;
				o->flags |= FLAG_SCRIPTONACTIVATE;
				o->xinertia = 0;
			}
		break;
	}
	
	o->yinertia += 0x40;
	LIMITX(0x400);
	LIMITY(0x5ff);
}


void ai_toroko_teleport_in(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->state = 1;
			o->timer = 0;
			o->flags &= ~FLAG_IGNORE_SOLID;		// this is set in npc.tbl, but uh, why?
		}
		case 1:
		{
			if (DoTeleportIn(o, 2))
			{
				o->frame = 1;
				o->state = 2;
				o->animtimer = 0;
			}
		}
		break;
		
		case 2:
			ANIMATE(2, 1, 4);
			
			if (o->blockd)
			{
				o->state = 4;
				o->frame = 6;		// tripping frame
				sound(SND_THUD);
			}
		break;
		
		case 4: break;			// knocked out
	}
	
	// fall unless teleporting
	if (o->state >= 2)
	{
		o->yinertia += 0x20;
		LIMITY(0x5ff);
	}
}

/*
void c------------------------------() {}
*/

void ai_npc_sue(Object *o)
{
	switch(o->state)
	{
		case 0:		// stand and blink
			o->timer = 0;
			o->frame = 0;
			o->xinertia = 0;
			o->sue.carried_by = NULL;
			randblink(o, 1, 4);
		break;
		
		case 3:		// walking
		case 4:		// walking
		case 5:		// face away
			ai_generic_npc(o);
		break;
		
		// got punched by Igor
		case 6:
			o->state = 7;
			o->frame = 7;
			o->timer = 0;
			sound(SND_ENEMY_SQUEAK);
		case 7:
			if (++o->timer > 10)
				o->state = 0;
		break;
		
		// got punched extra hard by Igor
		// flys through air backwards and crashes
		case 8:
			o->state = 9;
			o->frame = 7;
			o->timer = 0;
			sound(SND_ENEMY_SQUEAK);
			
			o->yinertia = -0x200;
			XMOVE(-0x400);
		case 9:
			if (++o->timer > 3 && o->blockd)
			{
				o->state = 10;
				o->dir ^= 1;
			}
		break;
		case 10:
			o->xinertia = 0;
			o->frame = 8;
		break;
		
		// punching the air (when she tells Igor "I'm not afraid of you!")
		case 11:
			o->state = 12;
			o->timer = 0;
			o->animframe = 0;
			o->animtimer = 0;
		case 12:
		{
			const static int punchframes[] = { 10, 0 };
			o->animate_seq(8, punchframes, 2);
		}
		break;
		
		// picked up & carried away by Igor
		case 13:
			o->frame = 11;
			o->xinertia = 0;
			o->yinertia = 0;
			o->state = 14;
			
			// find Igor
			o->sue.carried_by = FindObjectByID2(501);
			if (!o->sue.carried_by)
				staterr("-- Could not find entity carrying Sue (ID 501)");
		case 14:	// being carried--see aftermove routine
			o->frame = 9;
		break;
		
		// spawn red crystal and call it to us (Undead Core intro)
		case 15:
		{
			o->PushBehind(dr_create_red_crystal(o->x+(128 * CSFI), o->y));
			
			o->state = 16;
			o->xinertia = 0;
			o->frame = 0;
		}
		case 16:
		{
			crystal_xmark = o->x - (18 * CSFI);
			crystal_ymark = o->y - (8 * CSFI);
		}
		break;
		case 17:	// look up (still followed by red crystal)
		{
			o->xinertia = 0;
			o->frame = 12;
			
			crystal_xmark = o->x;
			crystal_ymark = o->y - (8 * CSFI);
		}
		break;
		
		// run away from DOCTOR_GHOST and hide behind player
		case 20:
		{
			o->state = 21;
			o->frame = 2;
			o->animtimer = 0;
		}
		case 21:
		{
			ANIMATE(2, 2, 5);
			XMOVE(0x400);
			
			if (o->x < player->x - (8 * CSFI))
			{
				o->dir = RIGHT;
				o->state = 0;
			}
		}
		break;
		
		// run, during "we've got to get out of here" post-undead core cutscene.
		case 30:
		{
			o->state = 31;
			o->frame = 2;
			o->animtimer = 0;
		}
		case 31:
		{
			ANIMATE(2, 2, 5);
			XMOVE(0x400);
		}
		break;
		
		case 40:	// she jumps off the island
		{
			o->state = 41;
			o->frame = 9;
			o->yinertia = -0x400;
		}
		break;
		
		/*default:
			staterr("-- Sue entered unhandled state %d (0x%02x)", o->state, o->state);
			exit(1);*/
	}
	
	o->yinertia += 0x40;
	LIMITX(0x400);
	LIMITY(0x5ff);
}

void aftermove_npc_sue(Object *o)
{
	// for being carried by igor
	if (o->sue.carried_by)
	{
		Object *link = o->sue.carried_by;
		
		o->x = ((link->x / CSFI) + sprites[link->sprite].frame[link->frame].dir[link->dir].actionpoint2.x) * CSFI;
		o->y = ((link->y / CSFI) + sprites[link->sprite].frame[link->frame].dir[link->dir].actionpoint2.y) * CSFI;
		
		o->dir = (link->dir ^ 1);
	}
}

void onspawn_npc_sue(Object *o)
{
	// fix a short falling glitch in Arthur's House cutscene
	// after Egg Corridor.
	// when we are <CNP'd to regular Sue from SUE_AT_COMPUTER,
	// as SUE_AT_COMPUTER is a taller sprite than regular Sue.
	o->SnapToGround();
}

void ai_sue_teleport_in(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			o->x += (6 * CSFI);
			//o->PushBehind(OBJ_MISERY_STAND);
			
			o->timer = 0;
			o->state = 1;
			o->frame = 9;	// her "hanging on a hook" frame
		}
		case 1:
		{
			if (DoTeleportIn(o, 2))
			{
				o->state = 2;
				o->timer = 0;
			}
		}
		break;
		
		case 2:
		{
			if (o->blockd)
			{
				o->frame = 13;	// crumpled
				o->state = 3;
				o->timer = 0;
				sound(SND_THUD);
			}
		}
		break;
	}
	
	// fall unless teleporting
	if (o->state >= 2)
	{
		o->yinertia += 0x20;
		LIMITY(0x5ff);
	}
}

/*
void c------------------------------() {}
*/

void ai_king(Object *o)
{
Object *sword = o->linkedobject;

	switch(o->state)
	{
		case 0:
			o->frame = 0; randblink(o, 1, 8);
			o->xinertia = o->yinertia = 0;
		break;
		
		case 5:			// he is knocked out
			o->frame = 3;
			o->xinertia = 0;
		break;
		
		case 6:			// hop in air then fall and knocked out
			o->state = 7;
			o->timer = 0;
			o->yinertia = -0x400;
		case 7:			// he falls and is knocked out
			o->frame = 2;
			XMOVE(0x200);
			o->yinertia += 0x40;
			if (o->timer++)
			{
				if (o->blockd)
				{
					o->state = 5;
				}
			}
		break;
		
		case 8:		// walking
			o->state = 9;
			o->frame = 4;
			o->animtimer = 0;
		case 9:
			ANIMATE(3, 4, 7);
			XMOVE(0x200);
		break;
		
		case 10:		// run
			o->state = 11;
			o->frame = 4;
			o->animtimer = 0;
		case 11:
			ANIMATE(2, 4, 7);
			XMOVE(0x400);
		break;
		
		case 20:		// pull out sword
			if (!sword)
			{
				sword = CreateObject(0, 0, OBJ_KINGS_SWORD);
				sword->linkedobject = o;
				o->linkedobject = sword;
			}
			
			o->frame = 0;
			o->state = 0;
		break;
		
		case 30:		// he goes flying in spec'd direction and smacks wall
			o->state = 31;
			o->timer = 0;
			o->frame = 2;
			XMOVE(0x600);
			o->yinertia = 0;
			o->nxflags &= ~NXFLAG_FOLLOW_SLOPE;		// necessary to keep him from going down slope...
		case 31:
			if (o->blockl)
			{
				o->dir = RIGHT;
				o->state = 7;
				o->timer = 0;
				o->yinertia = -0x400;
				o->xinertia = 0x200;
				sound(SND_LITTLE_CRASH);
				SmokeClouds(o, 4, 8, 8);
				o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			}
		break;
		
		case 40:		// he flickers away and is replaced by the Blade
			o->state = 41;
			o->timer = 0;
		case 41:
			o->invisible ^= 1;
			if (++o->timer > 100)
			{
				SmokeClouds(o, 4, 8, 8);
				o->state = 42;
				o->sprite = SPR_BLADE;
				o->frame = o->yinertia = o->invisible = 0;
			}
		break;
		case 42: break;
		
		case 60:		// jump (used when he lunges with sword)
			o->frame = 6;
			o->state = 61;
			o->yinertia = -0x5FF;
			o->xinertia = 0x400;
			if (sword) sword->carry.flip = 1;
		break;
		case 61:		// jumping
			o->yinertia += 0x80;
			if (o->blockd)
			{
				o->state = 0;
				o->xinertia = 0;
				if (sword) sword->carry.flip = 0;
			}
		break;
	}
	
	LIMITX(0x400);
	LIMITY(0x5FF);
}

/*
void c------------------------------() {}
*/

void ai_blue_robot(Object *o)
{
	o->frame = 0;
	randblink(o, 1, 4);
	
	o->yinertia += 0x40;
	LIMITY(0x5ff);
}

void ai_kanpachi_fishing(Object *o)
{
	if (!o->state)
	{
		o->SnapToGround();
		o->state = 1;
	}
	
	// open eyes when player comes near
	o->frame = (pdistlx((64 * CSFI)) && pdistly2((64 * CSFI), (16 * CSFI))) ? 1 : 0;
}

/*
void c------------------------------() {}
*/

void ai_professor_booster(Object *o)
{
	switch(o->state)
	{
		case 0:
			o->state = 1;
			o->timer = 0;
			o->xinertia = o->yinertia = 0;
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
		case 1:
			o->frame = 0;
			randblink(o, 1, 8, 120);
		break;
		
		case 30:	// teleporting in at Shelter
		{
			o->frame = 0;
			sound(SND_TELEPORT);
			
			// move into middle of teleporter
			o->x -= (TILE_W * CSFI);
			o->y += (TILE_H / 2) * CSFI;
			
			o->state++;
			o->timer = 0;
		}
		case 31:	// teleporting-in animation
		{
			if (DoTeleportIn(o, 2))
			{
				o->state++;
				o->timer = 0;
			}
		}
		break;
		case 32:	// wait before hop out of teleporter
		{
			if (++o->timer > 20)
			{
				o->state++;
				o->frame = 1;
			}
		}
		break;
		case 33:	// hopping out of teleporter
		{
			if (o->blockd)
			{
				o->state++;
				o->timer = 0;
				o->frame = 0;
			}
		}
		break;
		
		default:
			ai_generic_npc(o);
		break;
	}
	
	// no fall when teleporting in
	if (o->state != 31 && o->state != 32)
		o->yinertia += 0x40;
}

void ai_booster_falling(Object *o)
{
	switch(o->state)
	{
		case 0:			// sitting on ground after fall
			o->frame = 7;
		break;
		
		case 10:		// falling
		{
			o->frame = 8;
			o->yinertia += 0x40;
			LIMITY(0x5ff);
		}
		break;
		
		case 20:		// dying (flickering away)
		{
			sound(SND_TELEPORT);
			o->state = 21;
			o->timer = 0;
		}
		case 21:
		{
			o->invisible ^= 1;
			if (++o->timer > 100)
			{
				SmokeClouds(o, 4, 16, 16);
				o->Delete();
			}
		}
		break;
	}
}


/*
void c------------------------------() {}
*/

// shared between OBJ_KAZUMA_AT_COMPUTER and OBJ_SUE_AT_COMPUTER
void ai_npc_at_computer(Object *o)
{
	enum { INIT=0, TYPING, PAUSE_SLOUCH, PAUSE_UPRIGHT };
	
	switch(o->state)
	{
		case 0:
			o->SnapToGround();
			
			o->state = TYPING;
			o->frame = 0;
		
		case TYPING:
		{
			ANIMATE(2, 0, 1);
			
			if (!random(0, 80))
			{
				o->state = PAUSE_SLOUCH;
				o->frame = 1;
				o->timer = 0;
			}
			else if (!random(0, 120))
			{
				o->state = PAUSE_UPRIGHT;
				o->frame = 2;
				o->timer = 0;
			}
		}
		break;
		
		case PAUSE_SLOUCH:
		{
			if (++o->timer > 40)
			{
				o->state = PAUSE_UPRIGHT;
				o->frame = 2;
				o->timer = 0;
			}
		}
		break;
		
		case PAUSE_UPRIGHT:
		{
			if (++o->timer > 80)
			{
				o->state = TYPING;
				o->frame = 0;
				o->timer = 0;
			}
		}
		break;
	}
}

/*
void c------------------------------() {}
*/

void onspawn_generic_npc(Object *o)
{
	// these exceptions are because the Balcony helicopter has a state
	// which lets them ride in it and executing this messes it up.
	if (o->type != OBJ_SANTA && o->type != OBJ_CHACO)
	{
		o->SnapToGround();
	}
}

void ai_generic_npc(Object *o)
{
	if (o->state == 0)
	{
		FACEPLAYER;
	}
	
	ai_generic_npc_nofaceplayer(o);
}

void ai_generic_npc_nofaceplayer(Object *o)
{
	switch(o->state)
	{
		case 0:		// stand
		{
			o->nxflags |= NXFLAG_FOLLOW_SLOPE;
			o->frame = 0;
			o->xinertia = 0;
			o->yinertia = 0;
			randblink(o, 1, 8, 100);
		}
		break;
		
		case 3:		// walking
		case 4:
			npc_generic_walk(o, 3);
		break;
		
		case 5:		// face away
			o->frame = 6;
			o->xinertia = 0;
		break;
		
		case 8:		// walk (alternate state used by OBJ_NPC_JACK)
			if (o->type == OBJ_JACK)
			{
				o->state = 3;
				ai_generic_npc(o);
				return;
			}
		break;
		case 10:		// sleeping Chaco
			if (o->type == OBJ_CHACO)
			{
				o->dir = 0;
				o->frame = 6;
				o->flags &= ~ FLAG_SCRIPTONACTIVATE;
				o->state = 11;
			}
		break;
		case 11:		// sleeping Chaco
			if (o->type == OBJ_CHACO)
			{
				ai_zzzz_spawner(o);
			}
		break;
	}
	
}

void npc_generic_walk(Object *o, int basestate)
{
	if (o->state == basestate)
	{
		o->frame = 2;
		o->animtimer = 0;
		o->state++;
	}
	
	ANIMATE(3, 2, 5);
	XMOVE(0x200);
}






