#include "core.h"

#include "../stdai.h"
#include "../ai.h"
#include "../sym/smoke.h"
#include "../almond/almond.h"
#include "../../game.h"
#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../trig.h"
#include "../../sound/SoundManager.h"
#include "../../common/stat.h"
#include "../../common/misc.h"

#include "../../player.h"
#include "../../map.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"


/* ------------------------------------------------------------------------------------------ */
/*        CODE FOR THE CORE BOSS (ALMOND)													  */
/* ------------------------------------------------------------------------------------------ */
 
// these are the meanings of the various pieces inside pieces[] array
#define MC1				0			// minicores 1 through 5
#define MC2				1
#define MC3				2
#define MC4				3
#define MC5				4
#define CFRONT			5			// front half of the core body
#define CBACK			6			// back half of the core body

// states for the core
#define CORE_SLEEP			10
#define CORE_CLOSED			200
#define CORE_OPEN			210
#define CORE_GUST			220

// and the states for the minicores
#define MC_SLEEP			0
#define MC_THRUST			10
#define MC_CHARGE_FIRE		20
#define MC_FIRE				30
#define MC_FIRED			40
#define MC_RETREAT			50

// makes the core open his mouth and handles flashing red when hit
#define OPEN_MOUTH		\
{						\
	RunOpenMouth();	\
}

// makes the core close his mouth
#define CLOSE_MOUTH		\
{		\
	pieces[CFRONT]->frame = 2;		\
	pieces[CBACK]->frame = 0;		\
}

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_MINICORE, ai_minicore);
	ONTICK(OBJ_MINICORE_SHOT, ai_minicore_shot);
	
	AFTERMOVE(OBJ_CORE_BACK, ai_core_back);
	AFTERMOVE(OBJ_CORE_FRONT, ai_core_front);
	
	ONTICK(OBJ_CORE_GHOSTIE, ai_core_ghostie);
	ONTICK(OBJ_CORE_BLAST, ai_core_blast);
}

/*
void c------------------------------() {}
*/

static Object *CreateMinicore(Object *core)
{
Object *o;

	o = CreateObject(0, 0, OBJ_MINICORE);
	o->linkedobject = core;
	o->flags = (FLAG_SHOOTABLE | FLAG_INVULNERABLE | FLAG_IGNORE_SOLID);
	o->hp = 1000;
	o->state = MC_SLEEP;
	
	return o;
}


// called at the entry to the Core room.
// initilize all the pieces of the Core boss.
void CoreBoss::OnMapEntry(void)
{
	stat("CoreBoss::OnMapEntry");
	
	o = CreateObject(0, 0, OBJ_CORE_CONTROLLER);
	game.stageboss.object = o;
	
	o->state = 10;
	
	o->flags = (FLAG_SHOW_FLOATTEXT | FLAG_IGNORE_SOLID | FLAG_SCRIPTONDEATH);
	o->id2 = 1000;
	
	o->x = (1207 * CSFI);
	o->y = (212 * CSFI);
	o->xinertia = o->yinertia = 0;
	o->hp = 650;
	
	o->sprite = SPR_CORESHOOTMARKER;
	
	// spawn all the pieces in the correct z-order
	pieces[3] = CreateMinicore(o);
	pieces[4] = CreateMinicore(o);
	pieces[CFRONT] = CreateObject(0, 0, OBJ_CORE_FRONT);
	pieces[CBACK] = CreateObject(0, 0, OBJ_CORE_BACK);
	pieces[0] = CreateMinicore(o);
	pieces[1] = CreateMinicore(o);
	pieces[2] = CreateMinicore(o);
	
	// set up the front piece
	pieces[CFRONT]->sprite = SPR_CORE_FRONT;
	pieces[CFRONT]->state = CORE_SLEEP;
	pieces[CFRONT]->linkedobject = o;
	pieces[CFRONT]->flags |= (FLAG_IGNORE_SOLID | FLAG_INVULNERABLE);
	pieces[CFRONT]->frame = 2;			// mouth closed
	
	// set up our back piece
	pieces[CBACK]->sprite = SPR_CORE_BACK;
	pieces[CBACK]->state = CORE_SLEEP;
	pieces[CBACK]->linkedobject = o;
	pieces[CBACK]->flags |= (FLAG_IGNORE_SOLID | FLAG_INVULNERABLE);
	pieces[CBACK]->frame = 0;
	
	// set the positions of all the minicores
	pieces[0]->x = (o->x - 0x1000);
	pieces[0]->y = (o->y - 0x8000);
	
	pieces[1]->x = (o->x + 0x2000);
	pieces[1]->y = o->y;
	
	pieces[2]->x = (o->x - 0x1000);
	pieces[2]->y = (o->y + 0x8000);
	
	pieces[3]->x = (o->x - 0x6000);
	pieces[3]->y = (o->y + 0x4000);
	
	pieces[4]->x = (o->x - 0x6000);
	pieces[4]->y = (o->y - 0x4000);
	
	this->hittimer = 0;
}

void CoreBoss::OnMapExit()
{
	// ensure we are called no longer
	game.stageboss.object = NULL;
	o = NULL;
}

/*
void c------------------------------() {}
*/


void CoreBoss::Run()
{
bool do_thrust = false;
int i;

	if (!o) return;
	//stat("state = %d", o->state);
	
	switch(o->state)
	{
		case CORE_SLEEP:	break;			// core is asleep
		
		// Core's mouth is closed.
		// Core targets player point but does not update it during the state.
		// This is also the state set via BOA to awaken the core.
		case CORE_CLOSED:
		{
			o->state = CORE_CLOSED+1;
			o->timer = 0;
			
			StopWaterStream();
			o->xmark = player->x;
			o->ymark = player->y;
		}
		case CORE_CLOSED+1:
		{
			// open mouth after 400 ticks
			if (o->timer > 400)
			{
				if (++o->timer2 > 3)
				{	// every 3rd time do gusting left and big core blasts
					o->timer2 = 0;
					o->state = CORE_GUST;
				}
				else
				{
					o->state = CORE_OPEN;
				}
				
				do_thrust = true;
			}
		}
		break;
		
		// Core's mouth is open.
		// Core moves towards player, and updates the position throughout
		// the state (is "aggressive" about seeking him).
		// Core fires ghosties, and curly targets it.
		case CORE_OPEN:
		{
			o->state = CORE_OPEN+1;
			o->timer = 0;
			// gonna open mouth, so save the current HP so we'll
			// know how much damage we've taken this time.
			o->savedhp = o->hp;
		}
		case CORE_OPEN+1:
		{
			o->xmark = player->x;
			o->ymark = player->y;
			
			// must call constantly for red-flashing when hit
			OPEN_MOUTH;
			
			// hint curly to target us
			if ((o->timer % 64) == 1)
			{
				o->CurlyTargetHere();
			}
			
			// spawn ghosties
			if (o->timer < 200)
			{
				if ((o->timer % 20)==0)
				{
					CreateObject(o->x + (random(-48, -16) * CSFI), \
						     	 o->y + (random(-64, 64) * CSFI), \
							 	 OBJ_CORE_GHOSTIE);
				}
			}
			
			// close mouth when 400 ticks have passed or we've taken more than 200 damage
			if (o->timer > 400 || (o->savedhp - o->hp) >= 200)
			{
				o->state = CORE_CLOSED;
				CLOSE_MOUTH;
				do_thrust = true;
			}
		}
		break;
		
		
		case CORE_GUST:
		{
			o->state = CORE_GUST+1;
			o->timer = 0;
			
			StartWaterStream();
		}
		case CORE_GUST+1:
		{
			// spawn water droplet effects and push player
			Object *droplet = CreateObject(player->x + ((random(-50, 150) * CSFI)*2), \
								   		   player->y + (random(-160, 160) * CSFI),
								   		   OBJ_FAN_DROPLET);
			droplet->dir = LEFT;
			player->xinertia -= 0x20;
			
			OPEN_MOUTH;
			
			// spawn the big white blasts
			if (o->timer==300 || o->timer==350 || o->timer==400)
			{
				EmFireAngledShot(pieces[CFRONT], OBJ_CORE_BLAST, 0, 3 * CSFI);
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LIGHTNING_STRIKE);
			}
			
			if (o->timer > 400)
			{
				o->state = CORE_CLOSED;
				CLOSE_MOUTH;
				do_thrust = true;
			}
		}
		break;
		
		
		case 500:		// defeated!!
		{
			StopWaterStream();
			map.wlforcestate = WL_CALM;
			
			o->state = 501;
			o->timer = 0;
			o->xinertia = o->yinertia = 0;
			game.curlytarget.timeleft = 0;
			
			CLOSE_MOUTH;
			
			game.quaketime = 20;
			SmokeXY(pieces[CBACK]->x, pieces[CBACK]->CenterY(), 20, 128, 64);
			
			// tell all the MC's to retreat
			for(i=0;i<5;i++)
			{
				pieces[i]->flags &= ~(FLAG_SHOOTABLE & FLAG_INVULNERABLE);
				pieces[i]->state = MC_RETREAT;
			}
		}
		case 501:
		{
			o->timer++;
			if ((o->timer & 0x0f) != 0)
			{
				SmokeXY(pieces[CBACK]->x, pieces[CBACK]->CenterY(), 1, 64, 32);
			}
			
			if (o->timer & 2)
				o->x -= (1 * CSFI);
			else
				o->x += (1 * CSFI);
			
			#define CORE_DEATH_TARGET_X		0x7a000
			#define CORE_DEATH_TARGET_Y		0x16000
			o->xinertia += (o->x > CORE_DEATH_TARGET_X) ? -0x80 : 0x80;
			o->yinertia += (o->y > CORE_DEATH_TARGET_Y) ? -0x80 : 0x80;
		}
		break;
		
		case 600:			// teleported away by Misery
		{
			o->xinertia = 0;
			o->yinertia = 0;
			o->state++;
			//NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
			
			pieces[CFRONT]->clip_enable = pieces[CBACK]->clip_enable = 1;
			o->timer = sprites[pieces[CFRONT]->sprite].h;
		}
		case 601:
		{
			pieces[CFRONT]->display_xoff = pieces[CBACK]->display_xoff = random(-8, 8);
			
			pieces[CFRONT]->clipy2 = o->timer;
			pieces[CBACK]->clipy2 = o->timer;
			
			if (--o->timer < 0)
			{
				pieces[CFRONT]->invisible = true;
				pieces[CBACK]->invisible = true;
				
				// restore status bars
				game.stageboss.object = NULL;
				game.bossbar.object = NULL;
				o->Delete(); o = NULL;
				return;
			}
		}
		break;
	}
	
	
	if (do_thrust)
	{
		// tell all the minicores to jump to a new position
		for(i=0;i<5;i++)
		{
			pieces[i]->state = MC_THRUST;
		}
		
		quake(20);
		NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CORE_THRUST);
	}
	
	
	// fire the minicores in any awake non-dead state
	if (o->state >= CORE_CLOSED && o->state < 500)
	{
		o->timer++;
		
		// fire off each minicore sequentially...
		switch(o->timer)
		{
			case 80+0:   pieces[0]->state = MC_CHARGE_FIRE; break;
			case 80+30:  pieces[1]->state = MC_CHARGE_FIRE; break;
			case 80+60:  pieces[2]->state = MC_CHARGE_FIRE; break;
			case 80+90:  pieces[3]->state = MC_CHARGE_FIRE; break;
			case 80+120: pieces[4]->state = MC_CHARGE_FIRE; break;
		}
		
		// move main core towards a spot in front of target
		o->xinertia += (o->x > (o->xmark + (160 * CSFI))) ? -4 : 4;
		o->yinertia += (o->y > o->ymark - (o->Height() / 2)) ? -4 : 4;
	}
	
	// set up our shootable status--you never actually hit the core (CFRONT),
	// but if it's mouth is open, make us, the invisible controller object, shootable.
	if (pieces[CFRONT]->frame==2)
	{
		o->flags &= ~FLAG_SHOOTABLE;
		pieces[CFRONT]->flags |= FLAG_INVULNERABLE;
	}
	else
	{
		o->flags |= FLAG_SHOOTABLE;
		pieces[CFRONT]->flags &= ~FLAG_INVULNERABLE;
	}
	
	LIMITX(0x80);
	LIMITY(0x80);
}

/*
void c------------------------------() {}
*/

void CoreBoss::RunOpenMouth()
{
	// flash red when struck, else stay in Mouth Open frame
	pieces[CFRONT]->frame = 0;
	pieces[CBACK]->frame = 0;
	
	if (o->shaketime)
	{
		this->hittimer++;
		if (this->hittimer & 2)
		{
			pieces[CFRONT]->frame = 1;
			pieces[CBACK]->frame = 1;
		}
	}
	else
	{
		this->hittimer = 0;
	}
}

void CoreBoss::StartWaterStream(void)
{
	// bring the water up if it's not already up, but don't keep it up
	// if it's already been up on it's own because that's not fair
	if (map.wlstate == WL_DOWN)
		map.wlforcestate = WL_UP;
	
	game.quaketime = 100;
	NXE::Sound::SoundManager::getInstance()->startStreamSound(1000);
}

void CoreBoss::StopWaterStream(void)
{
	// bring the water down again if it's not already
	if (map.wlstate == WL_UP)
		map.wlforcestate = WL_CYCLE;
	
	NXE::Sound::SoundManager::getInstance()->stopLoopSfx();
}

/*
void c------------------------------() {}
*/

// the front (mouth) piece of the main core
void ai_core_front(Object *o)
{
	Object *core = o->linkedobject;
	if (!core) { o->Delete(); return; }
	
	o->x = core->x - 0x4800;
	o->y = core->y - 0x5e00;
}

// the back (unanimated) piece of the main core
void ai_core_back(Object *o)
{
	Object *core = o->linkedobject;
	if (!core) { o->Delete(); return; }
	
	o->x = core->x + (0x5800 - (8 * CSFI));
	o->y = core->y - 0x5e00;
}


/*
void c------------------------------() {}
*/


void ai_minicore(Object *o)
{
	Object *core = o->linkedobject;
	if (!core) { o->Delete(); return; }
	
	switch(o->state)
	{
		case MC_SLEEP:		// idle & mouth closed
			o->frame = 2;
			o->xmark = o->x;
			o->ymark = o->y;
		break;
		
		case MC_THRUST:			// thrust (move to random new pos)
			o->state = MC_THRUST+1;
			o->frame = 2;
			o->timer = 0;
			o->xmark = core->x + (random(-128, 32) * CSFI);
			o->ymark = core->y + (random(-64, 64) * CSFI);
		case MC_THRUST+1:
			if (++o->timer > 50)
			{
				o->frame = 0;
			}
		break;
		
		case MC_CHARGE_FIRE:			// charging for fire
			o->state = MC_CHARGE_FIRE+1;
			o->timer = 0;
		case MC_CHARGE_FIRE+1:			// flash blue
			o->timer++;
			o->frame = ((o->timer >> 1) & 1);
			if (o->timer > 20)
			{
				o->state = MC_FIRE;
			}
		break;
		
		case MC_FIRE:			// firing
			o->state = MC_FIRE+1;
			o->frame = 2;	// close mouth again
			o->timer = 0;
			o->xmark = o->x + (random(24, 48) * CSFI);
			o->ymark = o->y + (random(-4, 4) * CSFI);
		case MC_FIRE+1:
			if (++o->timer > 50)
			{
				o->state = MC_FIRED;
				o->frame = 0;
			}
			else if (o->timer==1 || o->timer==3)
			{
				// fire at player at speed (2 * CSFI) with 2 degrees of variance
				EmFireAngledShot(o, OBJ_MINICORE_SHOT, 2, 2 * CSFI);
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
			}
		break;
		
		
		case MC_RETREAT:		// defeated!
			o->state = MC_RETREAT+1;
			o->frame = 2;
			o->xinertia = o->yinertia = 0;
		case MC_RETREAT+1:		// retreat back into the abyss
			o->xinertia += 0x20;
			if (o->x > ((map.xsize*TILE_W) * CSFI) + 0x4000)
			{
				o->Delete();
			}
		break;
	}
	
	if (o->state < MC_RETREAT)
	{
		// jump back when shot
		if (o->shaketime)
		{
			o->xmark += 0x400;
		}
		
		o->x += (o->xmark - o->x) / 16;
		o->y += (o->ymark - o->y) / 16;
	}
	
	// don't let them kill us
	o->hp = 1000;
	
	// invincible when mouth is closed
	if (o->frame != 2)
		o->flags &= ~FLAG_INVULNERABLE;
	else
		o->flags |= FLAG_INVULNERABLE;
}

void ai_minicore_shot(Object *o)
{
	if (++o->timer2 > 150)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
	
	ai_animate2(o);
}
// shutter made noise when opening
// curly looks up at no 4

/*
void c------------------------------() {}
*/

void ai_core_ghostie(Object *o)
{
char hit = 0;

	if (o->xinertia > 0 && o->blockr) hit = 1;
	if (o->xinertia < 0 && o->blockl) hit = 1;
	if (o->yinertia > 0 && o->blockd) hit = 1;
	if (o->yinertia < 0 && o->blocku) hit = 1;
	
	o->xinertia -= 0x20;
	LIMITX(0x400);
	
	if (hit)
	{
		effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
		o->Delete();
	}
	
	ai_animate2(o);
}

void ai_core_blast(Object *o)
{
	if (++o->timer > 200) o->Delete();
	ANIMATE(2, 0, 1);
}
