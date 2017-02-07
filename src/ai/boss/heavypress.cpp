#include "heavypress.h"

#include "../stdai.h"
#include "../ai.h"
#include "../hell/ballos_priest.h"
#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../map.h"
#include "../../caret.h"
#include "../../sound/sound.h"
#include "../../common/misc.h"


#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../autogen/sprites.h"


// position in Hell (note this is the center position, because there is a draw point)
#define HELL_X			(160<<CSF)
#define HELL_Y			(74<<CSF)
#define HELL_FLOOR		(160<<CSF)
#define HELL_BOTTOM		(480<<CSF)

// positions of stuff for Passageway scene
#define PWAY_X			(160<<CSF)		// X position of corridor
#define PWAY_TOP		(64<<CSF)		// starting position for falling scene
#define PWAY_BOTTOM		(413<<CSF)		// resting position after fall


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_HP_LIGHTNING, ai_hp_lightning);
}

void HeavyPress::OnMapEntry(void)
{
	o = CreateObject(HELL_X, HELL_Y, OBJ_HEAVY_PRESS);
	game.stageboss.object = o;
	shield_left = shield_right = NULL;
	
	objprop[o->type].hurt_sound = SND_ENEMY_HURT_COOL;
	objprop[o->type].shaketime = 8;
	
	o->flags = (FLAG_SHOW_FLOATTEXT | FLAG_SCRIPTONDEATH | \
			    FLAG_SOLID_BRICK | FLAG_IGNORE_SOLID);
	
	o->damage = 10;
	o->hp = 700;
	o->id2 = 1000;	// defeated script
	
	// setup bboxes
	center_bbox = sprites[o->sprite].frame[0].dir[0].pf_bbox;
	fullwidth_bbox = sprites[o->sprite].frame[2].dir[0].pf_bbox;
	
	sprites[o->sprite].bbox = fullwidth_bbox;
}

/*
void c------------------------------() {}
*/

void HeavyPress::Run()
{
	if (!o) return;
	//AIDEBUG;
	
	run_defeated();
	run_passageway();
	
	switch(o->state)
	{
		// fight begin (script-triggered)
		case 100:
		{
			// get coordinates of the blocks to destroy as it's uncovered
			uncover_left = (o->Left() >> CSF) / TILE_W;
			uncover_right = (o->Right() >> CSF) / TILE_W;
			uncover_y = ((o->Bottom() >> CSF) / TILE_H) + 1;
			
			// create shielding objects for invincibility on either side
			// don't use puppet 1 because Deleet's use that when they explode.
			shield_left = CreateObject(o->x, o->y, OBJ_HEAVY_PRESS_SHIELD);
			shield_left->sprite = SPR_BBOX_PUPPET_2;
			sprites[shield_left->sprite].bbox = fullwidth_bbox;
			sprites[shield_left->sprite].bbox.x2 = center_bbox.x1 - 1;
			
			shield_right = CreateObject(o->x, o->y, OBJ_HEAVY_PRESS_SHIELD);
			shield_right->sprite = SPR_BBOX_PUPPET_3;
			sprites[shield_right->sprite].bbox = fullwidth_bbox;
			sprites[shield_right->sprite].bbox.x1 = center_bbox.x2 + 1;
			
			// then switch to small pfbox where we're only hittable in the center
			o->frame = 0;
			sprites[o->sprite].bbox = center_bbox;
			
			o->flags |= FLAG_SHOOTABLE;
			o->flags &= ~FLAG_INVULNERABLE;
			
			o->state = 101;
			o->timer = -100;	// pause a moment before Butes come
		}
		case 101:
		{	// fire lightning
			CreateObject(o->x, o->y+0x7800, OBJ_HP_LIGHTNING);
			o->state = 102;
		}
		case 102:
		{
			// spawn butes on alternating sides
			switch(o->timer++)
			{
				case 0:
				case 160:
					CreateObject(MAPX(17), MAPY(15), OBJ_BUTE_FALLING)->dir = UP;
				break;
				
				case 80:
				case 240:
					CreateObject(MAPX(3), MAPY(15), OBJ_BUTE_FALLING)->dir = UP;
				break;
				
				case 298:
					// fire lightning next frame
					o->state = 101;
					o->timer = 0;
				break;
			}
			
			// uncover as it's damaged
			if (o->hp < (uncover_y * 70) && uncover_y > 1)
			{
				uncover_y--;
				sound(SND_BLOCK_DESTROY);
				
				for(int x=uncover_left;x<=uncover_right;x++)
					map_ChangeTileWithSmoke(x, uncover_y, 0, 4);
			}
		}
		break;
	}
	
	// flashing when hit. note, setting frame to 0 would mess up the
	// defeated sequence, except that shaketime is always expired by the time
	// the eye is opened, and this way we can still flash on the last shot.
	if (o->shaketime)
		o->frame = (o->shaketime & 2) ? 3 : 0;
}


void HeavyPress::run_defeated()
{
	switch(o->state)
	{
		// defeated (set by ondeath script, after a brief pause)
		case 500:
		{
			o->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE);
			o->state = 501;
			o->timer = 0;
			o->frame = 0;
			
			if (shield_left) { shield_left->Delete(); shield_left = NULL; }
			if (shield_right) { shield_right->Delete(); shield_right = NULL; }
			sprites[o->sprite].bbox = fullwidth_bbox;
			
			// get rid of enemies--the butes can stay, though.
			KillObjectsOfType(OBJ_HP_LIGHTNING);
			KillObjectsOfType(OBJ_ROLLING);
		}
		case 501:
		{
			o->timer++;
			if ((o->timer % 16) == 0)
			{
				sound(SND_BLOCK_DESTROY);
				SmokePuff(o->x + random(-60<<CSF, 60<<CSF),
						  o->y + random(-40<<CSF, 40<<CSF));
			}
			
			// eye opens
			switch(o->timer)
			{
				case 95: o->frame = 1; break;	// partially open
				case 98: o->frame = 2; break;	// fully open
				case 101:
				{
					o->state = 502;		// fall
					o->damage = 127;
					o->flags &= ~FLAG_SOLID_BRICK;
				}
				break;
			}
		}
		break;
		
		// falling (haven't hit floor yet)
		case 502:
		{
			// hit floor: bounce. The floor tiles are not
			// solid to NPC's so we can't use blockd.
			if (o->y > HELL_FLOOR)
			{
				o->state = 503;
				o->yinertia = -0x200;
				
				// kill floor
				int y = (o->Bottom() >> CSF) / TILE_H;
				for(int x=uncover_left-1;x<=uncover_right+1;x++)
				{
					map_ChangeTileWithSmoke(x, y, 0, 1, true);
					sound(SND_BLOCK_DESTROY);
				}
				
				if (o->y > HELL_BOTTOM)
					o->state = 0;
			}
		}
		case 503:	// falling, and hit floor already
		{
			o->yinertia += 0x40;
		}
		break;
	}
}


// states for when it's in the Passageway
void HeavyPress::run_passageway()
{
	switch(o->state)
	{
		// "dead"/smoking on Passageway floor
		// note they may also be coming back from Statue Room.
		// script-triggered.
		case 20:
		{
			o->state = 21;
			o->x = PWAY_X;
			o->y = PWAY_BOTTOM;
			o->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE | FLAG_SOLID_BRICK);
			o->damage = 0;
		}
		case 21:
		{
			if ((++o->timer % 16) == 0)
			{
				int x = random(o->Left(), o->Right());
				int y = random(o->Top(), o->Bottom());
				SmokePuff(x, y);
				effect(x, y, EFFECT_BOOMFLASH);
			}
		}
		break;
		
		// falling through Passageway when you jump down hole in post-defeat scene.
		// script-triggered.
		case 30:
		{
			o->state = 31;
			o->frame = 2;	// eye open
			COPY_PFBOX;		// ensure that we are using the full-width bbox
			
			o->x = PWAY_X;
			o->y = PWAY_TOP;
		}
		case 31:
		{
			o->y += (4 << CSF);
			
			if (o->y >= PWAY_BOTTOM)
			{
				o->y = PWAY_BOTTOM;
				sound(SND_MISSILE_HIT);
				
				o->frame = 0;
				for(int i=0;i<5;i++)
					SmokePuff(random(o->Left(), o->Right()), o->Bottom());
				
				o->state = 20;	// continue smoking
			}
		}
		break;
	}
}



/*
void c------------------------------() {}
*/

void ai_hp_lightning(Object *o)
{
	switch(o->state)
	{
		case 0:
		{
			sound(SND_TELEPORT);
			o->sprite = SPR_HP_CHARGE;
			o->state = 1;
		}
		case 1:
		{
			ANIMATE(0, 0, 2);
			
			if (++o->timer > 50)
			{
				o->state = 10;
				o->animtimer = 0;
				o->frame = 3;
				o->damage = 10;
				o->sprite = SPR_HP_LIGHTNING;
				sound(SND_LIGHTNING_STRIKE);
				
				// smoke on floor where it struck
				SmokeXY(o->CenterX(), o->Bottom() - (7<<CSF), 3, 0, 0);
			}
		}
		break;
		
		case 10:
		{
			ANIMATE_FWD(2);
			if (o->frame > 6)
				o->Delete();
		}
		break;
	}
	
}



















