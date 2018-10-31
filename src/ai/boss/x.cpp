#include "x.h"
#include "../stdai.h"
#include "../ai.h"

#include "../sym/smoke.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../tsc.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../common/stat.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../map.h"
#include "../../screeneffect.h"
#include "../../graphics/graphics.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


#define STATE_X_APPEAR				1		// script-triggered: must stay constant
#define STATE_X_FIGHT_BEGIN			10		// script-triggered: must stay constant
#define STATE_X_TRAVEL				20
#define STATE_X_BRAKE				30
#define STATE_X_OPEN_DOORS			40
#define STATE_X_FIRE_TARGETS		50
#define STATE_X_FIRE_FISHIES		60
#define STATE_X_CLOSE_DOORS			70
#define STATE_X_EXPLODING			80

#define STATE_DOOR_OPENING			10		// makes the doors open
#define STATE_DOOR_OPENING_PARTIAL	20		// makes the doors open part-way
#define STATE_DOOR_CLOSING			30		// closes the doors
#define STATE_DOOR_FINISHED			40		// doors are finished moving

#define STATE_TREAD_STOPPED			20
#define STATE_TREAD_RUN				30
#define STATE_TREAD_BRAKE			40

#define STATE_FISHSPAWNER_FIRE		10
#define STATE_TARGET_FIRE			10

#define DOORS_OPEN_DIST			(32 * CSFI)		// how far the doors open
#define DOORS_OPEN_FISHY_DIST	(20 * CSFI)		// how far the doors open during fish-missile phase

// the treads start moving at slightly different times
// which we change direction, etc.
static const int tread_turnon_times[] = { 4, 8, 10, 12 };


INITFUNC(AIRoutines)
{
	ONTICK(OBJ_X_FISHY_MISSILE, ai_x_fishy_missile);
	ONTICK(OBJ_X_DEFEATED, ai_x_defeated);
	
	ONDEATH(OBJ_X_TARGET, ondeath_x_target);
	ONDEATH(OBJ_X_MAINOBJECT, ondeath_x_mainobject);
}

void XBoss::OnMapEntry(void)
{
	stat("XBoss::OnMapEntry()");
	
	memset(&X, 0, sizeof(X));
	memset(&body, 0, sizeof(body));
	memset(&treads, 0, sizeof(treads));
	memset(&internals, 0, sizeof(internals));
	memset(&doors, 0, sizeof(doors));
	memset(&targets, 0, sizeof(targets));
	memset(&fishspawners, 0, sizeof(fishspawners));
	npieces = 0;
	
	mainobject = CreateObject(0, 0, OBJ_X_MAINOBJECT);
	mainobject->sprite = SPR_NULL;
	
	
	game.stageboss.object = mainobject;
}

void XBoss::OnMapExit()
{
	// we'll let the map loader code handle deleting all our pieces.
	// here's just a good-form failsafe to ensure XBoss::Run() runs no more.
	mainobject = NULL;
	game.stageboss.object = NULL;
}

/*
void c------------------------------() {}
*/

void XBoss::Run()
{
Object *o = mainobject;
int i;

	if (!mainobject) return;
	if (o->state == 0 || (!X.initilized && o->state != STATE_X_APPEAR))
	{
		o->hp = 1;
		o->x = -(SCREEN_WIDTH * CSFI);
		return;
	}
	
	switch(o->state)
	{
		// script triggered us to initilize/appear
		// (there is a hvtrigger, right before player first walks by us
		// and sees us inactive, which sends us this ANP).
		case STATE_X_APPEAR:
		{
			if (!X.initilized)
			{
				Init();
				X.initilized = true;
			}
		}
		break;
		
		// script has triggered the fight to begin
		case STATE_X_FIGHT_BEGIN:
		{
			o->timer = 0;
			o->state++;
		}
		case STATE_X_FIGHT_BEGIN+1:
		{
			if (++o->timer > 100)
			{
				FACEPLAYER;
				o->timer = 0;
				o->state = STATE_X_TRAVEL;
			}
		}
		break;
		
		// starts the treads and moves us in the currently-facing direction
		case STATE_X_TRAVEL:
		{
			// count number of times we've traveled, we brake
			// and attack every third time.
			o->timer2++;
			
			o->timer = 0;
			o->state++;
		}
		case STATE_X_TRAVEL+1:
		{
			o->timer++;
			
			// trigger the treads to start moving,
			// and put them slightly out of sync with each-other.
			for(int i=0;i<4;i++)
			{
				if (o->timer == tread_turnon_times[i])
				{
					treads[i]->state = STATE_TREAD_RUN;
					treads[i]->dir = o->dir;
				}
			}
			
			if (o->timer > 120)
			{
				// time to attack? we attack every 3rd travel
				// if so skid to a stop, that's the first step.
				if (o->timer2 >= 3)
				{
					o->timer2 = 0;
					
					o->dir ^= 1;
					o->state = STATE_X_BRAKE;
					o->timer = 0;
				}
				else
				{
					// passed player? skid and turn around.
					if ((o->dir == RIGHT && o->x > player->x) || \
					 	(o->dir == LEFT  && o->x < player->x))
					{
						o->dir ^= 1;
						o->state = STATE_X_TRAVEL;
					}
				}
			}
		}
		break;
		
		// skidding to a stop in preparation to attack
		case STATE_X_BRAKE:
		{
			o->timer = 0;
			o->state++;
		}
		case STATE_X_BRAKE+1:
		{
			o->timer++;
			
			// trigger the treads to start braking,
			// and put them slightly out of sync with each-other.
			for(int i=0;i<4;i++)
			{
				if (o->timer == tread_turnon_times[i])
				{
					treads[i]->state = STATE_TREAD_BRAKE;
					treads[i]->dir = o->dir;
				}
			}
			
			if (o->timer > 50)
			{
				o->state = STATE_X_OPEN_DOORS;
				o->timer = 0;
			}
		}
		break;
		
		// doors opening to attack
		case STATE_X_OPEN_DOORS:
		{
			o->timer = 0;
			o->savedhp = o->hp;
			
			// select type of attack depending on where we are in the battle
			if (!AllTargetsDestroyed())
			{
				SetStates(doors, 2, STATE_DOOR_OPENING);
				o->state = STATE_X_FIRE_TARGETS;
			}
			else
			{
				SetStates(doors, 2, STATE_DOOR_OPENING_PARTIAL);
				o->state = STATE_X_FIRE_FISHIES;
			}
		}
		break;
		
		// firing targets (early battle)
		case STATE_X_FIRE_TARGETS:
		{
			if (doors[0]->state == STATE_DOOR_FINISHED)
			{
				doors[0]->state = 0;
				SetStates(targets, 4, STATE_TARGET_FIRE);
			}
			
			if (++o->timer > 300 || AllTargetsDestroyed())
			{
				o->state = STATE_X_CLOSE_DOORS;
				o->timer = 0;
			}
		}
		break;
		
		// firing fishy missiles (late battle)
		case STATE_X_FIRE_FISHIES:
		{
			if (doors[0]->state == STATE_DOOR_FINISHED)
			{
				doors[0]->state = 0;
				
				SetStates(fishspawners, 4, STATE_FISHSPAWNER_FIRE);
				internals->flags |= FLAG_SHOOTABLE;
			}
			
			if (++o->timer > 300 || (o->savedhp - o->hp) > 200)
			{
				o->state = STATE_X_CLOSE_DOORS;
				o->timer = 0;
			}
		}
		break;
		
		// doors closing after attack
		case STATE_X_CLOSE_DOORS:
		{
			o->timer = 0;
			o->state++;
			
			SetStates(doors, 2, STATE_DOOR_CLOSING);
		}
		case STATE_X_CLOSE_DOORS+1:
		{
			if (doors[0]->state == STATE_DOOR_FINISHED)
			{
				doors[0]->state = 0;
				
				// just turn off everything for both types of attacks;
				// turning off the attack type that wasn't enabled isn't harmful.
				SetStates(targets, 4, 0);
				SetStates(fishspawners, 4, 0);
				internals->flags &= ~FLAG_SHOOTABLE;
			}
			
			if (++o->timer > 50)
			{
				FACEPLAYER;
				o->state = STATE_X_TRAVEL;
				o->timer = 0;
			}
		}
		break;
		
		// exploding
		case STATE_X_EXPLODING:
		{
			SetStates(fishspawners, 4, 0);
			KillObjectsOfType(OBJ_X_FISHY_MISSILE);
			
			game.tsc->StartScript(1000);
			o->timer = 0;
			o->state++;
		}
		case STATE_X_EXPLODING+1:
		{
			game.quaketime = 2;
			o->timer++;
			
			if ((o->timer % 8) == 0)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_HURT_BIG);
			
			SmokePuff(o->CenterX() + (random(-72, 72) * CSFI),
					  o->CenterY() + (random(-64, 64) * CSFI));
			
			if (o->timer > 100)
			{
				starflash.Start(o->CenterX(), o->CenterY());
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPLOSION1);
				o->timer = 0;
				o->state++;
			}
		}
		break;
		case STATE_X_EXPLODING+2:
		{
			game.quaketime = 40;
			if (++o->timer > 50)
			{
				CreateObject(o->x, o->y - (24 * CSFI), OBJ_X_DEFEATED);
				DeleteMonster();
				return;
			}
		}
		break;
	}
	
	// call AI for all tread pieces
	for(i=0;i<4;i++)
	{
		run_tread(i);
		run_fishy_spawner(i);
	}
}

// moved this to aftermove so xinertia on treads is already applied
// when we calculate the main object position.
void XBoss::RunAftermove()
{
Object *o = mainobject;
int i;

	if (!mainobject || mainobject->state == 0 || !X.initilized)
		return;
	
	// main object pulled along as treads move
	int tread_center = (treads[UL]->x + treads[UR]->x + \
					 	treads[LL]->x + treads[LR]->x) / 4;
	o->x += (tread_center - o->x) / 16;
	
	run_internals();
	
	for(i=0;i<4;i++)
	{
		run_body(i);
		run_target(i);
	}
	
	for(i=0;i<2;i++)
	{
		run_door(i);
	}
}

void ondeath_x_mainobject(Object *internals)
{
	// do nothing really, this function is just there to override
	// the default so we are not destroyed--our 0 HP level will
	// be noticed in run_internals() and trigger the defeat sequence.
	internals->flags &= ~FLAG_SHOOTABLE;
}

/*
void c------------------------------() {}
*/

void XBoss::run_tread(int index)
{
	Object *o = treads[index];
	
	switch(o->state)
	{
		case 0:
		{
			o->flags |= (FLAG_SOLID_BRICK | FLAG_INVULNERABLE | FLAG_NOREARTOPATTACK);
			o->state = STATE_TREAD_STOPPED;
		}
		case STATE_TREAD_STOPPED:
		{
			o->frame = 0;
			o->damage = 0;
			o->flags &= ~FLAG_BOUNCY;
		}
		break;
		
		case STATE_TREAD_RUN:
		{
			o->flags |= FLAG_BOUNCY;
			o->timer = 0;
			o->frame = 2;
			o->animtimer = 0;
			
			o->state++;
		}
		case STATE_TREAD_RUN+1:
		{
			ANIMATE(0, 2, 3);
			XACCEL(0x20);
			
			if (++o->timer > 30)
			{
				o->flags &= ~FLAG_BOUNCY;
				o->frame = 0;
				o->animtimer = 0;
				o->state++;
			}
		}
		break;
		case STATE_TREAD_RUN+2:
		{
			ANIMATE(1, 0, 1);
			XACCEL(0x20);
			
			o->timer++;
		}
		break;
		
		case STATE_TREAD_BRAKE:
		{
			o->frame = 2;
			o->animtimer = 0;
			
			o->flags |= FLAG_BOUNCY;
			o->state++;
		}
		case STATE_TREAD_BRAKE+1:
		{
			ANIMATE(0, 2, 3);
			XACCEL(0x20);
			
			if ((o->dir == RIGHT && o->xinertia > 0) || \
				(o->dir == LEFT && o->xinertia < 0))
			{
				o->xinertia = 0;
				o->state = STATE_TREAD_STOPPED;
			}
		}
		break;
	}
	
	// make motor noise
	switch(o->state)
	{
		case STATE_TREAD_RUN+1:
		case STATE_TREAD_BRAKE+1:
		{
			if (o->timer & 1)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MOTOR_SKIP);
		}
		break;
		
		case STATE_TREAD_RUN+2:
		{
			if ((o->timer % 4) == 1)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MOTOR_RUN);
		}
		break;
	}
	
	// determine if player is in a position where he could get run over.
	if (o->state > STATE_TREAD_STOPPED && o->xinertia != 0)
	{
		if (abs(player->y - o->CenterY()) <= (5 * CSFI))
			o->damage = 10;
		else
			o->damage = 0;
	}
	else
	{
		o->damage = 0;
	}
	
	LIMITX(0x400);
}

void XBoss::run_body(int i)
{
	// set body position based on main object position and
	// our linked tread position. first get the center point we should be at...
	body[i]->x = (mainobject->x + treads[i]->x) / 2;
	body[i]->y = (mainobject->y + treads[i]->y) / 2;
	
	// ...and place our center pixel at those coordinates.
	int dx = (sprites[body[i]->sprite].w / 2) - 8;
	int dy = (sprites[body[i]->sprite].h / 2) - 8;
	body[i]->x -= dx * CSFI;
	body[i]->y -= dy * CSFI;
	
	// tweaks
	if (i == UL || i == LL)
	{
		body[i]->x -= (6 * CSFI);
	}
	else
	{
		body[i]->x += (7 * CSFI);
	}
	
	if (i == LL || i == LR)
	{
		body[i]->y += (8 * CSFI);
	}

}

void XBoss::run_internals()
{
	internals->x = mainobject->x;
	internals->y = mainobject->y;
	
	// select frame
	if (internals->shaketime & 2)
	{
		internals->frame = 1;
	}
	else
	{
		internals->frame = (mainobject->state < 10) ? 2 : 0;
	}
	
	// link damage to main object
	if (internals->hp < 1000)
	{
		mainobject->DealDamage(1000 - internals->hp);
		internals->hp = 1000;
	}
	
	// trigger explosion sequence when monster defeated
	if (mainobject->hp <= 0 && mainobject->state < STATE_X_EXPLODING)
	{
		mainobject->shaketime = 150;
		mainobject->state = STATE_X_EXPLODING;
	}
}

void XBoss::run_door(int index)
{
	Object *o = doors[index];
	
	switch(o->state)
	{
		// doors opening all the way
		case STATE_DOOR_OPENING:
		{
			o->xmark += (1 * CSFI);
			
			if (o->xmark >= DOORS_OPEN_DIST)
			{
				o->xmark = DOORS_OPEN_DIST;
				o->state = STATE_DOOR_FINISHED;
			}
		}
		break;
		
		// doors opening partially for fish-missile launchers to fire
		case STATE_DOOR_OPENING_PARTIAL:
		{
			o->xmark += (1 * CSFI);
			
			if (o->xmark >= DOORS_OPEN_FISHY_DIST)
			{
				o->xmark = DOORS_OPEN_FISHY_DIST;
				o->state = STATE_DOOR_FINISHED;
			}
		}
		break;
		
		// doors closing
		case STATE_DOOR_CLOSING:
		{
			o->xmark -= (1 * CSFI);
			if (o->xmark <= 0)
			{
				o->xmark = 0;
				o->state = STATE_DOOR_FINISHED;
			}
		}
		break;
		
		// this is a signal to the main object that the doors
		// are finished with the last command.
		case STATE_DOOR_FINISHED:
		break;
	}
	
	// set position relative to main object.
	// doors open in opposite directions.
	if (o->dir == LEFT) o->x = (mainobject->x - o->xmark);
				   else o->x = (mainobject->x + o->xmark);
	
	o->y = mainobject->y;
}

void XBoss::run_fishy_spawner(int index)
{
	Object *o = fishspawners[index];
	
	switch(o->state)
	{
		case STATE_FISHSPAWNER_FIRE:
		{
			o->timer = 20 + (index * 20);
			o->state++;
		}
		case STATE_FISHSPAWNER_FIRE+1:
		{
			if (o->timer)
			{
				o->timer--;
				break;
			}
			
			// keep appropriate position relative to main object
			//                               UL          UR         LL         LR
			static const int xoffs[]   = { -64 * CSFI,  76 * CSFI, -64 * CSFI,  76 * CSFI };
			static const int yoffs[]   = {  27 * CSFI,  27 * CSFI, -16 * CSFI, -16 * CSFI };
			o->x = (mainobject->x + xoffs[index]);
			o->y = (mainobject->y + yoffs[index]);
			
			Object *missile = CreateObject(o->x, o->y, OBJ_X_FISHY_MISSILE);
			missile->dir = index;
			
			NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
			o->timer = 120;
		}
		break;
	}
}

void XBoss::run_target(int index)
{
	Object *o = targets[index];
	
	// has this target been destroyed?
	// (we don't really kill the object until the battle is over,
	// to avoid having to deal with dangling pointers).
	if (o->invisible)
		return;
	
	switch(o->state)
	{
		case 0:
			o->flags &= ~FLAG_SHOOTABLE;
			o->frame &= 3;
			o->state = 1;
		break;
		
		case STATE_TARGET_FIRE:
		{
			o->timer = 40 + (index * 10);
			o->flags |= FLAG_SHOOTABLE;
			o->state++;
		}
		case STATE_TARGET_FIRE+1:
		{
			if (--o->timer <= 16)
			{
				// flash shortly before firing
				if (o->timer & 2) o->frame |= 4;
							 else o->frame &= 3;
				
				if (o->timer <= 0)
				{
					o->timer = 40;
					EmFireAngledShot(o, OBJ_GAUDI_FLYING_SHOT, 2, 0x500);
					NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
				}
			}
		}
		break;
	}
	
	// keep appropriate position on internals
	//                               UL          UR         LL         LR
	static const int xoffs[] = { -22 * CSFI,  28 * CSFI, -15 * CSFI,  17 * CSFI };
	static const int yoffs[] = { -16 * CSFI, -16 * CSFI,  14 * CSFI,  14 * CSFI };
	
	o->x = internals->x + xoffs[index];
	o->y = internals->y + yoffs[index];
}

void ondeath_x_target(Object *o)
{
	SmokeClouds(o, 8, 8, 8);
	NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LITTLE_CRASH);
	
	o->flags &= ~FLAG_SHOOTABLE;
	o->invisible = true;
}

/*
void c------------------------------() {}
*/

void XBoss::Init()
{
int i;

	mainobject->hp = 700;
	mainobject->state = 1;
	mainobject->x = (128 * TILE_W) * CSFI;
	mainobject->y = (200 * CSFI);
	mainobject->flags = FLAG_IGNORE_SOLID | FLAG_SHOW_FLOATTEXT;
	
	// put X behind the flying gaudis
	mainobject->PushBehind(lowestobject);
	
	// create body pieces
	for(i=3;i>=0;i--)
	{
		body[i] = CreatePiece(0, 0, OBJ_X_BODY);
		body[i]->dir   = (i == UL || i == LL) ? LEFT : RIGHT;
		body[i]->frame = (i == LL || i == LR) ? 1 : 0;
	}
	
	// create treads
	for(i=0;i<4;i++)
	{
		int x = (i == UL || i == LL) ? 0xf8000 : 0x108000;
		int y = (i == UL || i == UR) ? 0x12000 : (0x20000 - (16 * CSFI));
		int sprite = (i == UL || i == UR) ? SPR_X_TREAD_UPPER : SPR_X_TREAD_LOWER;
		
		treads[i] = CreateTread(x, y, sprite);
		treads[i]->smushdamage = 10;
	}
	
	// create internals
	internals = CreatePiece(0, 0, OBJ_X_INTERNALS);
	internals->hp = 1000;
	internals->flags &= ~FLAG_SHOW_FLOATTEXT;
	
	// create targets
	for(i=0;i<4;i++)
	{
		targets[i] = CreatePiece(0, 0, OBJ_X_TARGET);
		targets[i]->sprite = SPR_X_TARGETS;
		targets[i]->frame = i;
		targets[i]->hp = 60;
		targets[i]->flags &= ~FLAG_SHOW_FLOATTEXT;
	}
	
	// create fishy-missile shooters
	for(i=0;i<4;i++)
	{
		fishspawners[i] = CreatePiece(0, 0, OBJ_X_FISHY_SPAWNER);
		fishspawners[i]->sprite = SPR_NULL;
		fishspawners[i]->invisible = true;
		fishspawners[i]->flags = 0;
	}
	
	// create doors
	for(i=0;i<2;i++)
	{
		doors[i] = CreatePiece(0, 0, OBJ_X_DOOR);
		doors[i]->sprite = SPR_X_DOOR;
		doors[i]->dir = i;
	}
	
	sprites[SPR_X_DOOR].frame[0].dir[LEFT].drawpoint.x = 40;
	sprites[SPR_X_DOOR].frame[0].dir[LEFT].drawpoint.y = 16;
	sprites[SPR_X_DOOR].frame[0].dir[RIGHT].drawpoint.x = -9;
	sprites[SPR_X_DOOR].frame[0].dir[RIGHT].drawpoint.y = 16;
}

// create an object and record it as a piece of the monster
// so we can delete all the pieces later via DeleteMonster().
Object *XBoss::CreatePiece(int x, int y, int object)
{
	Object *piece = CreateObject(x, y, object);
	piecelist[npieces++] = piece;
	piece->PushBehind(mainobject);
	return piece;
}

// create an object of type OBJ_X_TREAD and give it the specified sprite.
Object *XBoss::CreateTread(int x, int y, int sprite)
{
	Object *tread = CreatePiece(x, y, OBJ_X_TREAD);
	tread->sprite = sprite;
	return tread;
}

// delete all pieces of the monster
void XBoss::DeleteMonster()
{
	for(int i=0;i<npieces;i++)
		piecelist[i]->Delete();
	
	mainobject->Delete();
	mainobject = NULL;
	game.stageboss.object = NULL;
}

// return true if all the targets behind the doors have been destroyed.
bool XBoss::AllTargetsDestroyed()
{
	for(int i=0;i<4;i++)
	{
		if (!targets[i]->invisible)
			return false;
	}
	
	return true;
}

/*
void c------------------------------() {}
*/

// sets state on an array on objects
void XBoss::SetStates(Object *objects[], int nobjects, int state)
{
	for(int i=0;i<nobjects;i++)
		objects[i]->state = state;
}

// sets direction on an array on objects
void XBoss::SetDirs(Object *objects[], int nobjects, int dir)
{
	for(int i=0;i<nobjects;i++)
		objects[i]->dir = dir;
}

/*
void c------------------------------() {}
*/

void ai_x_fishy_missile(Object *o)
{
	if (o->state == 0)
	{
		static const int angle_for_dirs[] = { 160, 224, 96, 32 };
		
		o->angle = angle_for_dirs[o->dir];
		o->dir = RIGHT;
		
		o->state = 1;
	}
	
	vector_from_angle(o->angle, 0x400, &o->xinertia, &o->yinertia);
	int desired_angle = GetAngle(o->x, o->y, player->x, player->y);
	
	if (o->angle >= desired_angle)
	{
		if ((o->angle - desired_angle) < 128)
		{
			o->angle--;
		}
		else
		{
			o->angle++;
		}
	}
	else
	{
		if ((o->angle - desired_angle) < 128)
		{
			o->angle++;
		}
		else
		{
			o->angle--;
		}
	}
	
	// smoke trails
	if (++o->timer2 > 2)
	{
		o->timer2 = 0;
		Caret *c = effect(o->ActionPointX(), o->ActionPointY(), EFFECT_SMOKETRAIL_SLOW);
		c->xinertia = -o->xinertia >> 2;
		c->yinertia = -o->yinertia >> 2;
	}
	
	o->frame = (o->angle + 16) / 32;
	if (o->frame > 7) o->frame = 7;
}


// this is the cat that falls out after you defeat him
void ai_x_defeated(Object *o)
{
	o->timer++;
	if ((o->timer % 4) == 0)
	{
		SmokeClouds(o, 1, 16, 16);
	}
	
	switch(o->state)
	{
		case 0:
		{
			SmokeClouds(o, 8, 16, 16);
			o->state = 1;
		}
		case 1:
		{
			if (o->timer > 50)
			{
				o->state = 2;
				o->xinertia = -0x100;
			}
			
			// three-position shake
			o->x += (o->timer & 2) ? (1 * CSFI) : -(1 * CSFI);
		}
		break;
		
		case 2:
		{
			o->yinertia += 0x40;
			if (o->y > (map.ysize * TILE_H) * CSFI) o->Delete();
		}
		break;
	}
}
