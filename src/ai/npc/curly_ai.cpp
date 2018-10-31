#include "curly_ai.h"
#include "../ai.h"
#include "../stdai.h"
#include "../../ObjManager.h"
#include "../../p_arms.h"
#include "../../sound/SoundManager.h"
#include "../../common/misc.h"

#include "../../game.h"
#include "../../player.h"
#include "../../map.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"
#include "../../autogen/sprites.h"



#define CAI_INIT			20			// ANP'd to this by the entry script in Lab M
#define CAI_START			21			// ANP'd to this by Almond script
#define CAI_RUNNING			22
#define CAI_KNOCKEDOUT		40			// knocked out at beginning of Almond battle
#define CAI_ACTIVE			99

INITFUNC(AIRoutines)
{
	ONTICK(OBJ_CURLY_AI, ai_curly_ai);
	
	ONTICK(OBJ_CAI_GUN, ai_cai_gun);
	AFTERMOVE(OBJ_CAI_GUN, aftermove_cai_gun);
	AFTERMOVE(OBJ_CAI_WATERSHIELD, aftermove_cai_watershield);
}

// curly that fights beside you
void ai_curly_ai(Object *o)
{
int xdist, ydist;
int xlimit;
char reached_p;
int otiley;
char seeking_player = 0;
char wantdir;

/*
	debug("Curly Console");
	debug("TGT: [%d,%d] %d", o->xmark/CSFI, o->ymark/CSFI, game.curlytarget.timeleft);
	debug("State: %d", o->state);
	debug("");
	debug("RPT %d", o->curly.reachptimer);
	debug("TJT %d", o->curly.tryjumptime);
	debug("IJT %d:%d", o->curly.impjumptime, o->curly.impjump);
	game.debug.god = 1;
if (inputs[DEBUGKEY7]) o->state=999;
*/
	
	// put these here so she'll spawn the shield immediately, even while she's still
	// knocked out. otherwise she wouldn't have it turned on in the cutscene if the
	// player defeats the core before she gets up. I know that's unlikely but still.
	if (!o->curly.spawned_watershield)
	{
		Object *shield = CreateObject(0, 0, OBJ_CAI_WATERSHIELD);
		shield->sprite = SPR_WATER_SHIELD;
		shield->linkedobject = o;
		
		o->BringToFront();				// curly in front of monsters,
		shield->BringToFront();			// and shield in front of curly
		
		o->curly.spawned_watershield = 1;
	}
	
	switch(o->state)
	{
		case 0:
			o->xinertia = 0;
			o->yinertia += 0x20;
		break;
		
		case CAI_INIT:			// set to this by an ANP in Maze M
			o->x = player->x;
			o->y = player->y;
		case CAI_START:			// set here after she stops being knocked out in Almond
		{
			o->invisible = 0;
			o->xmark = o->x;
			o->ymark = o->y;
			o->dir = player->dir;
			o->state = CAI_ACTIVE;
			o->timer = 0;
			
			// spawn her gun
			Object *gun = CreateObject(0, 0, OBJ_CAI_GUN);
			gun->linkedobject = o;
			gun->PushBehind(o);
			
			if (player->weapons[WPN_MGUN].hasWeapon)
				o->curly.gunsprite = SPR_PSTAR;
			else
				o->curly.gunsprite = SPR_MGUN;
		}
		break;
		
		case CAI_KNOCKEDOUT:
		{
			o->timer = 0;
			o->state = CAI_KNOCKEDOUT+1;
			o->frame = 15;
		}
		case CAI_KNOCKEDOUT+1:
		{
			if (++o->timer > 1000)
			{	// start fighting
				o->state = CAI_START;
			}
			else if (o->timer > 750)
			{	// stand up
				o->flags &= ~FLAG_SCRIPTONACTIVATE;
				o->frame = 0;
			}
		}
		break;
	}
	
	if (o->state != CAI_ACTIVE) return;
	
	// first figure out where our target is
	
	// hack in case player REALLY leaves her behind. this works because of the way
	// the level is in a Z shape. first we check to see if the player is on the level below ours.
	if ((player->y > o->y && ((player->y - o->y) > 160 * CSFI)) || o->state==999)
	{
		// if we're on the top section, head all the way to right, else if we're on the
		// middle section, head for the trap door that was destroyed by the button
		otiley = (o->y / CSFI) / TILE_H;
		
		game.curlytarget.timeleft = 0;
		
		if (otiley < 22)
		{
			o->xmark = ((126 * TILE_W) + 8) * CSFI;		// center of big chute on right top
		}
		else if (otiley > 36 && otiley < 47)
		{	// fell down chute in center of middle section
			// continue down chute, don't get hung up on sides
			o->xmark = (26 * TILE_W) * CSFI;
		}
		else if (otiley >= 47)
		{	// bottom section - head for exit door
			// (this shouldn't ever execute, though, because player can't be lower than this)
			o->xmark = (81 * TILE_W) * CSFI;
			seeking_player = 1;		// stop when reach exit door
		}
		else
		{	// on middle section
			o->xmark = ((7 * TILE_W) + 8) * CSFI;		// trap door which was destroyed by switch
		}
		
		o->ymark = o->y;
	}
	else
	{
		// if we get real far away from the player leave the enemies alone and come find him
		if (!pdistlx(160 * CSFI)) game.curlytarget.timeleft = 0;
		
		// if we're attacking an enemy head towards the enemy else return to the player
		if (game.curlytarget.timeleft)
		{
			o->xmark = game.curlytarget.x;
			o->ymark = game.curlytarget.y;
			
			game.curlytarget.timeleft--;
			if (game.curlytarget.timeleft==60 && random(0, 2)==0) CaiJUMP(o);
		}
		else
		{
			o->xmark = player->x;
			o->ymark = player->y;
			seeking_player = 1;
		}
	}
	
	// do not fall off the middle railing in Almond
	if (game.curmap == STAGE_ALMOND)
	{
		#define END_OF_RAILING		(((72*TILE_W)-8) * CSFI)
		if (o->xmark > END_OF_RAILING)
		{
			o->xmark = END_OF_RAILING;
		}
	}
	
	// calculate distance to target
	xdist = abs(o->x - o->xmark);
	ydist = abs(o->y - o->ymark);
	
	// face target. I used two seperate IF statements so she doesn't freak out at start point
	// when her x == xmark.
	wantdir = o->dir;
	if (o->x < o->xmark) wantdir = RIGHT;
	if (o->x > o->xmark) wantdir = LEFT;
	if (wantdir != o->dir)
	{
		if (++o->curly.changedirtimer > 4)
		{
			o->curly.changedirtimer = 0;
			o->dir = wantdir;
		}
	}
	else o->curly.changedirtimer = 0;
	
	// if trying to return to the player then go into a rest state when we've reached him
	reached_p = 0;
	if (seeking_player && xdist < (32 * CSFI) && ydist < (64 * CSFI))
	{
		if (++o->curly.reachptimer > 80)
		{
			o->xinertia *= 7;
			o->xinertia /= 8;
			o->frame = 0;
			reached_p = 1;
		}
	}
	else o->curly.reachptimer = 0;
	
	if (!reached_p)		// if not at rest walk towards target
	{
		// walking animation
		if (++o->animtimer > 4)
		{
			o->animtimer = 0;
			if (++o->animframe > 3) o->animframe = 0;
		}
		
		// walk towards target
		if (o->x > o->xmark) o->xinertia -= 0x20;
		if (o->x < o->xmark) o->xinertia += 0x20;
		o->frame = o->animframe;
		
		// jump if we hit a wall
		if ((o->blockr && o->xinertia > 0) || (o->blockl && o->xinertia < 0))
		{
			if (++o->curly.blockedtime > 8)
			{
				CaiJUMP(o);
			}
		}
		else o->curly.blockedtime = 0;
		
		// if our target gets really far away (like p is leaving us behind) and
		// the above jumping isn't getting us anywhere, activate the Improbable Jump
		if ((o->blockl || o->blockr) && xdist > (80 * CSFI))
		{
			if (++o->curly.impjumptime > 60)
			{
				if (o->blockd)
				{
					CaiJUMP(o);
					o->curly.impjumptime = -100;
					o->curly.impjump = 1;
				}
			}
		}
		else o->curly.impjumptime = 0;
		
		// if we're below the target try jumping around randomly
		if (o->y > o->ymark && (o->y - o->ymark) > (16 * CSFI))
		{
			if (++o->curly.tryjumptime > 20)
			{
				o->curly.tryjumptime = 0;
				if (random(0, 1)) CaiJUMP(o);
			}
		}
		else o->curly.tryjumptime = 0;
	}
	else o->animtimer = o->animframe = 0;		// reset walk anim
	
	// force jump/fall frames
	if (o->yinertia < 0) o->frame = 3;
	else if (!o->blockd) o->frame = 1;
	else if (o->x==o->xmark) o->frame = 0;
	
	// the improbable jump - when AI gets confused, just cheat!
	// jump REALLY high by reducing gravity until we clear the wall
	if (o->curly.impjump > 0)
	{
		o->yinertia += 0x10;
		// deactivate Improbable Jump once we clear the wall or hit the ground
		if (o->dir==LEFT && !o->blockl) o->curly.impjump--;
		if (o->dir==RIGHT && !o->blockr) o->curly.impjump--;
		if (o->yinertia > 0 && o->blockd) o->curly.impjump--;
	}
	else o->yinertia += 0x33;
	
	// slow down when we hit bricks
	if (o->blockl || o->blockr)
	{
		// full stop if on ground, partial stop if in air
		xlimit = o->blockd ? 0x000:0x180;
		
		if (o->blockl)
		{
			if (o->xinertia < -xlimit) o->xinertia = -xlimit;
		}
		else if (o->xinertia > xlimit)		// we don't have to test blockr because we already know one or the other is set and that it's not blockl
		{
			o->xinertia = xlimit;
		}
	}
	
	// look up/down at target
	o->curly.look = 0;
	if (!reached_p || abs(o->y - player->y) > (48 * CSFI))
	{
		if (o->y > o->ymark && ydist >= (12 * CSFI) && (!seeking_player || ydist >= (80 * CSFI))) o->curly.look = UP;
		else if (o->y < o->ymark && !o->blockd && ydist >= (80 * CSFI)) o->curly.look = DOWN;
	}
	
	if (o->curly.look == UP) o->frame += 4;
	else if (o->curly.look == DOWN) o->frame += 8;
	
	LIMITX(0x300);
	LIMITY(0x5ff);
}

static void CaiJUMP(Object *o)
{
	if (o->blockd && !o->blocku)
	{
		o->yinertia = random(-0x600, -0x300);
		o->frame = 3;
		NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_PLAYER_JUMP);
	}
}

/*
void c------------------------------() {}
*/

void ai_cai_gun(Object *o)
{
Object *curly = o->linkedobject;
Object *shot;
uint8_t fire;
int x, y, dir;
#define SMALLDIST		(32 * CSFI)
#define BIGDIST			(160 * CSFI)

	if (!curly) { o->Delete(); return; }
	
	o->frame = 0;
	if (curly->curly.look)
	{
		o->sprite = (curly->curly.gunsprite + 1);
		if (curly->curly.look==DOWN) o->frame = 1;
	}
	else
	{
		o->sprite = curly->curly.gunsprite;
	}
	
	if (game.curlytarget.timeleft)
	{
		// fire when we get close to the target
		if (!curly->curly.look)
		{	// firing LR-- fire when lined up vertically and close by horizontally
			fire = ((abs(o->x - game.curlytarget.x) <= BIGDIST) && (abs(o->y - game.curlytarget.y) <= SMALLDIST));
		}
		else
		{	// firing vertically-- fire when lined up horizontally and close by vertically
			fire = ((abs(o->x - game.curlytarget.x) <= SMALLDIST) && (abs(o->y - game.curlytarget.y) <= BIGDIST));
		}
		
		if (fire)
		{
			// get coordinate of our action point
			x = (o->ActionPointX() - o->DrawPointX());
			y = (o->ActionPointY() - o->DrawPointY());
			dir = curly->curly.look ? curly->curly.look : o->dir;
			
			if (curly->curly.gunsprite==SPR_MGUN)
			{	// she has the Machine Gun
				if (!o->timer)
				{
					o->timer2 = random(2, 6);		// no. shots to fire
					o->timer = random(40, 50);
					o->animtimer = 0;
				}
				
				if (o->timer2)
				{	// create the MGun blast
					if (!o->animtimer)
					{
						FireLevel23MGun(x, y, 2, dir);
						
						o->animtimer = 5;
						o->timer2--;
					}
					else o->animtimer--;
				}
			}
			else
			{	// she has the Polar Star
				if (!o->timer)
				{
					o->timer = random(4, 16);
					if (random(0, 10)==0) o->timer += random(20, 30);
					
					// create the shot
					shot = CreateObject(x, y, OBJ_POLAR_SHOT);
					SetupBullet(shot, x, y, B_PSTAR_L3, dir);
				}
			}
		}
	}
	
	if (o->timer) o->timer--;
}

void aftermove_cai_gun(Object *o)
{
	Object *curly = o->linkedobject;
	if (curly)
	{
		o->x = curly->ActionPointX();
		o->y = curly->ActionPointY();
		o->dir = curly->dir;
	}
}


// curly's air bubble when she goes underwater
void aftermove_cai_watershield(Object *o)
{
	Object *curly = o->linkedobject;
	if (!curly)
	{
		o->Delete();
		return;
	}
	
	static const Point cwp = { 8, 5 };
	if (curly->GetAttributes(&cwp, 1, NULL) & TA_WATER)
	{
		o->invisible = false;
		o->x = curly->x;
		o->y = curly->y;
		
		o->frame = (++o->timer & 2) ? 1 : 0;
	}
	else
	{
		o->invisible = true;
		o->timer = o->frame = 0;
	}
}
