
#include "nx.h"
#include "player.h"
#include "playerstats.h"
#include "caret.h"
#include "ObjManager.h"
#include "map.h"
#include "tsc.h"
#include "input.h"
#include "game.h"
#include "sound/sound.h"
#include "common/misc.h"
#include "ai/weapons/whimstar.h"
#include "p_arms.h"
#include "ai/sym/smoke.h"
#include "autogen/sprites.h"
#include "graphics/sprites.h"
using namespace Sprites;
#include "settings.h"
#include "screeneffect.h"
#include "inventory.h"


Player *player = NULL;
//static void InitWeapon(int wpn, int l1, int l2, int l3, int maxammo=0);

bool pinputs[INPUT_COUNT];
bool lastpinputs[INPUT_COUNT];

static void InitWeapon(int wpn, int l1, int l2, int l3, int maxammo=0)
{
	player->weapons[wpn].max_xp[0] = l1;
	player->weapons[wpn].max_xp[1] = l2;
	player->weapons[wpn].max_xp[2] = l3;
	player->weapons[wpn].maxammo = maxammo;
}


void PInitFirstTime()
{
	player->dir = RIGHT;
	player->hp = player->maxHealth = 3;
	player->nxflags |= NXFLAG_FOLLOW_SLOPE;
	
	player->ninventory = 0;
	
	memset(player->weapons, 0, sizeof(player->weapons));
	
	InitWeapon(WPN_POLARSTAR, 		10, 20, 10);
	InitWeapon(WPN_MGUN, 			30, 40, 10, 100);
	InitWeapon(WPN_MISSILE,			10, 20, 10, 10);
	InitWeapon(WPN_FIREBALL,		10, 20, 20);
	InitWeapon(WPN_BLADE,			15, 18, 0);
	InitWeapon(WPN_BUBBLER,			10, 20, 5);
	InitWeapon(WPN_SUPER_MISSILE,	30, 60, 10, 10);
	InitWeapon(WPN_SNAKE,			30, 40, 16);
	InitWeapon(WPN_SPUR,			40, 60, 200);
	InitWeapon(WPN_NEMESIS,			1,  1,  0);
	
	player->weapons[WPN_MGUN].SetFireRate(6, 6, 6);
	player->weapons[WPN_MGUN].SetRechargeRate(5, 5, 5);
	
	player->weapons[WPN_BUBBLER].SetFireRate(0, 7, 7);
	player->weapons[WPN_BUBBLER].SetRechargeRate(20, 2, 2);
	
	player->curWeapon = WPN_NONE;
	
	if (player->XPText) delete player->XPText;
	player->XPText = new FloatText(SPR_WHITENUMBERS);
	
	// initialize player repel points
	PInitRepel();
}




void InitPlayer(void)
{
	player->lookaway = false;
	player->walking = false;
	player->dead = false;
	player->drowned = false;
	player->disabled = false;
	
	player->hurt_time = 0;
	player->hurt_flash_state = 0;
	player->water_shield_frame = 0;
	player->movementmode = MOVEMODE_NORMAL;
	player->inputs_locked_lasttime = true;
	
	player->booststate = BOOST_OFF;
	player->lastbooststate = BOOST_OFF;
	player->boosterfuel = BOOSTER_FUEL_QTY;
	
	player->xinertia = 0;
	player->yinertia = 0;
	
	player->riding = NULL;
	player->lastriding = NULL;
	player->cannotride = NULL;
	
	player->DamageText->Reset();
	player->XPText->Reset();
	statusbar.xpflashcount = 0;
	
	PResetWeapons();
	PSelectSprite();
	
	// this prevents a splash if we start underwater, and prevents us
	// from drowning immediately since our air isn't yet set up
	player->touchattr = TA_WATER;
	player->airleft = 1000;
	player->airshowtimer = 0;
}

Player::~Player()
{
	if (XPText)
	{
		delete XPText;
		XPText = NULL;
	}
}

/*
void c------------------------------() {}
*/

void HandlePlayer(void)
{
	// freeze player for the split-second between <TRA to a new map and the
	// start of the on-entry script for that map. (Fixes: player could shoot during
	// end sequence if he holds key down).
	if (game.switchstage.mapno != -1)
		return;
	
	PUpdateInput();
	
	if (!player->dead)
	{
		PHandleAttributes();			// handle special tile attributes
		PHandleSolidMushyObjects();		// handle objects like bugs marked "solid / mushy"
		
		PDoWeapons();	// p_arms.cpp
		PDoHurtFlash();
		
		switch((inputs[DEBUG_MOVE_KEY] && settings->enable_debug_keys) ? MOVEMODE_DEBUG : \
			   player->movementmode)
		{
			case MOVEMODE_NORMAL:
			{
				PDoBooster();
				PDoBoosterEnd();
				PDoWalking();
				PDoLooking();
				PDoJumping();
				PDoFalling();
				PSelectFrame();
			}
			break;
			
			case MOVEMODE_ZEROG:		// Ironhead battle/UNI 1
			{
				PHandleZeroG();
			}
			break;
			
			case MOVEMODE_DEBUG:
			{
				player->xinertia = player->yinertia = 0;
				player->blockl = player->blockr = player->blockd = player->blocku = 0;
				
				if (inputs[DOWNKEY]) player->y += 0x1000;
				if (inputs[UPKEY]) player->y -= 0x1000;
				if (inputs[LEFTKEY]) { player->x -= 0x1000; player->dir = LEFT; }
				if (inputs[RIGHTKEY]) { player->x += 0x1000; player->dir = RIGHT; }
				
				map_scroll_jump(player->x, player->y);
				
				player->frame = 2;
			}
			break;
			
			default:
			{
				player->xinertia = player->yinertia = 0;
			}
			break;
		}
		
		// handle some special features, like damage and bouncy, of
		// 100% solid objects such as moving blocks. It's put at the end
		// so that we can see the desired inertia of the player before
		// it's canceled out by any block points that are set. That way
		// we can tell if the player is trying to move into it.
		PHandleSolidBrickObjects();
	}
	
	// apply inertia
	PDoPhysics();
	
	// thud sound when land on some objects
	if (player->riding && !player->lastriding &&
		(player->riding->nxflags & NXFLAG_THUD_ON_RIDING))
	{
	    rumble(0.3,100);
		sound(SND_THUD);
	}
}

// player aftermove routine
void HandlePlayer_am(void)
{
	//debug("xinertia: %s", strhex(player->xinertia));
	//debug("yinertia: %s", strhex(player->yinertia));
	//debug("booststate: %d", player->booststate);
	//debug("y: %d", player->y / CSFI);
	//debug("riding %x", player->riding);
	//debug("block: %d%d%d%d", player->blockl, player->blockr, player->blocku, player->blockd);
	
	// if player is riding some sort of platform apply it's inertia to him
	if (player->riding)
	{
		player->apply_xinertia(player->riding->xinertia);
		player->apply_yinertia(player->riding->yinertia);
	}
	
	// keep player out of blocks "SMB1 style"
	PDoRepel();
	
	// handle landing and bonking head
	if (player->blockd && player->yinertia > 0)
	{
		if (player->yinertia > 0x400 && !player->hide)
		{
		    rumble(0.3,100);
			sound(SND_THUD);
		}
		
		player->yinertia = 0;
		player->jumping = 0;
	}
	else if (player->blocku && player->yinertia < 0)
	{
		// he behaves a bit differently when bonking his head on a
		// solid-brick object vs. bonking his head on the map.
		
		// bonk-head star effect
		if (player->yinertia < -0x200 && !player->hide && \
			player->blocku == BLOCKED_MAP)
		{
			sound(SND_BONK_HEAD);
			rumble(0.4,200);
			effect(player->CenterX(), player->y, EFFECT_BONKPLUS);
		}
		
		// bounces off ceiling with booster 0.8
		if (player->booststate == BOOST_08)
		{
			player->yinertia = 0x200;
		}
		else if (player->bopped_object && player->bopped_object->yinertia != 0)
		{
			// no clear yinertia when bop head on OBJ_BLOCK_MOVEV in labyrinth.
		}
		else
		{
			player->yinertia = 0;
		}
		
		player->jumping = false;
	}
	
	player->lastwalking = player->walking;
	player->lastriding = player->riding;
	player->inputs_locked_lasttime = player->inputs_locked;
	memcpy(lastpinputs, pinputs, sizeof(lastpinputs));
}

/*
void c------------------------------() {}
*/

void PDoPhysics(void)
{
	if (player->xinertia > 0x5ff)  player->xinertia = 0x5ff;
	if (player->xinertia < -0x5ff) player->xinertia = -0x5ff;
	if (player->yinertia > 0x5ff)  player->yinertia = 0x5ff;
	if (player->yinertia < -0x5ff) player->yinertia = -0x5ff;
	
	if (player->blockd && player->yinertia > 0)
		player->yinertia = 0;
	
	player->apply_yinertia(player->yinertia);
	
	// if xinertia is less than the decel speed then maintain the value but don't actually
	// move anything. It seems a bit odd...but that's the best I can figure to make it
	// behave like the original.
	if (player->xinertia > player->decelspeed || player->xinertia < -player->decelspeed)
	{
		player->apply_xinertia(player->xinertia);
	}
}

void PUpdateInput(void)
{
int i;

	if (player->inputs_locked || player->disabled)
	{
		memset(pinputs, 0, sizeof(pinputs));
	}
	else
	{
		memcpy(pinputs, inputs, sizeof(pinputs));
		
		// prevent jumping/shooting when leaving a messagebox
		if (player->inputs_locked_lasttime)
		{
			for(i=0;i<INPUT_COUNT;i++)
				lastpinputs[i] |= pinputs[i];
		}
		
		// allow entering inventory
		if (justpushed(INVENTORYKEY))
		{
			if (!game.frozen && !player->dead && game.tsc->GetCurrentScript() == -1)
			{
				game.setmode(GM_INVENTORY);
			}
		}
		
		// Map System
		if (justpushed(MAPSYSTEMKEY) && (FindInventory(ITEM_MAP_SYSTEM)!=-1))
		{
			if (!game.frozen && !player->dead && game.tsc->GetCurrentScript() == -1)
			{
				if (fade.getstate() == FS_NO_FADE && game.switchstage.mapno == -1)
				{
					game.setmode(GM_MAP_SYSTEM, game.mode);
				}
			}
		}
	}
}


// handles tile attributes of tiles player is touching
void PHandleAttributes(void)
{
static const Point pattrpoints[] = { {8, 8}, {8, 14} };
static const Point hurt_bottom_attrpoint =   {8, 7};
unsigned int attr;
int tile;

	// get attributes of tiles player it touching.
	// first, we'll check the top pattrpoint alone; this is the point at
	// which you go underwater, when that point is lower than the water level.
	// ** There is a spot in Labyrinth W just after the Shop where the positioning
	// of this point is a minor element in the gameplay, and so it must be set
	// correctly. If set too high you will not be underwater after climbing up the
	// small slope and you can just jump over the wall that you shouldn't be able to.
	attr = player->GetAttributes(&pattrpoints[0], 1, &tile);
	
	// water handler -- water uses only the top pattrpoint
	if (attr & TA_WATER)
	{
		// check if we just entered the water
		if (!(player->touchattr & TA_WATER))
		{
			// splash on entering water quick enough
			if ((player->yinertia > 0x200 && !player->blockd) || \
				(player->xinertia < -0x200 || player->xinertia > 0x200))
			{
				int x = player->CenterX();
				int y = player->CenterY();
				int splashtype = !(player->touchattr & TA_HURTS_PLAYER) ? \
									OBJ_WATER_DROPLET : OBJ_LAVA_DROPLET;
				
				for(int i=0;i<8;i++)
				{
					Object *o = CreateObject(x + (random(-8, 8) * CSFI), y, splashtype);
					o->xinertia = random(-0x200, 0x200) + player->xinertia;
					o->yinertia = random(-0x200, 0x80) - (player->yinertia >> 1);
				}
				
				sound(SND_SPLASH);
			}
		}
		
		// setup physics constants for water
		player->walkspeed = 0x196;
		player->fallspeed = 0x2ff;
		
		player->fallaccel = 0x28;
		player->jumpfallaccel = 0x10;
		
		player->walkaccel = 0x2a;
		player->jumpwalkaccel = 0x10;
		
		player->decelspeed = 0x19;
		// was set at 0x280 but I believe that makes it impossible to clear one of the long
		// spike jumps in River
		player->jumpvelocity = 0x280; //0x2c0;
		
		// decrement air left
		if (player->equipmask & EQUIP_AIRTANK)
		{
			player->airleft = 1000;
			player->airshowtimer = 0;
		}
		else
		{
			player->airshowtimer = 60;
			if (!player->drowned)
			{
				if (!player->inputs_locked) player->airleft--;
				
				if (player->airleft <= 0 && !game.debug.god)
				{	// player drowned
					// if flag 4000 is set, then we do not drown, but are in the Almond
					// level after Core battle, and should instead execute script 1100.
					if (game.flags[4000])
					{	// "your senses dim and the world grows dark"
						game.tsc->StartScript(1100);
					}
					else
					{	// nope sorry buddy, no such luck this time
						Object *o = CreateObject(player->x, player->y, OBJ_NULL);
						o->state = 1; // so ai doesn't fire
						o->sprite = SPR_PDROWNED;
						o->dir = player->dir;
						
						killplayer(SCRIPT_DROWNED);
					}
					
					player->drowned = 1;
				}
			}
		}
	}
	else
	{
		// setup normal physics constants
		player->walkspeed = 0x32c;////0x030e;
		player->fallspeed = 0x5ff;
		
		player->fallaccel = 0x50;
		player->jumpfallaccel = 0x20;
		
		player->walkaccel = 0x55;
		player->jumpwalkaccel = 0x20;
		
		player->decelspeed = 0x33;
		player->jumpvelocity = 0x500;
		
		// reset air supply
		player->airleft = 1000;
		if (player->airshowtimer) player->airshowtimer--;
	}
	
	// add in the bottom pattrpoint, but don't let it set the "water" bit.
	// only the top pattrpoint can set "water".
	attr |= (player->GetAttributes(&pattrpoints[1], 1, &tile) & ~TA_WATER);
	
	// If the tile has "hurt" bit, we recheck it with the the different bottom attrpoint.
	// This fixes bottom spikes in water level, last cave... Standart bottom attrpoint
	// allows intersection with spike only for 1 pixel, but origianl game allows 8 pixels 
	// of safe intersection.
	if (attr & TA_HURTS_PLAYER)
	{
		attr &= ~TA_HURTS_PLAYER;
		attr |= (player->GetAttributes(&hurt_bottom_attrpoint, 1, &tile) & ~TA_WATER);
	}
	
	if (attr & TA_HURTS_PLAYER)
		hurtplayer(10);
	
	// water current/wind:
	// for water currents--get the sum total of several points on the player to see
	// all the directions he's getting blown around at (support multiple directions)
	DoWaterCurrents();
	player->touchattr = attr;
}

// handes player being blown around by water currents
void DoWaterCurrents(void)
{
static Point currentpoints[] = { {7, 8},
								 {1, 2}, {1, 8}, {1, 14},
								 {7, 2}, {7, 14},
								 {15,2}, {15, 8}, {15, 14} };
int i;
static const int current_dir[] = { LEFTMASK, UPMASK, RIGHTMASK, DOWNMASK };
uint8_t currentmask;
int tile;

	// check each point in currentpoints[] for a water current, and if found,
	// add it to the list of directions we're being blown
	currentmask = 0;
	for(i=0;i<9;i++)
	{
		//DebugCrosshair(player->x+(currentpoints[i].x * CSFI),player->y+(currentpoints[i].y * CSFI), 255,0,0);
		
		if (player->GetAttributes(&currentpoints[i], 1, &tile) & TA_CURRENT)
		{
			currentmask |= current_dir[tilecode[tile] & 3];
		}
		
		// if the center point (the first one) has no current, then don't
		// bother checking the rest. as during 90% of the game you are NOT underwater.
		if (!currentmask) return;
	}
	
	// these constants are very critical for Waterway to work properly.
	// please be careful with them.
	if (currentmask & LEFTMASK)  player->xinertia -= 0x88;
	if (currentmask & RIGHTMASK) player->xinertia += 0x88;
	if (currentmask & UPMASK)    player->yinertia -= 0x80;
	if (currentmask & DOWNMASK)  player->yinertia += 0x50;
}


void PDoWalking(void)
{
int walk_accel;
int limit;
	
	walk_accel = (player->blockd) ? player->walkaccel : player->jumpwalkaccel;
	
	// walking/moving
	if (pinputs[LEFTKEY] || pinputs[RIGHTKEY])
	{
		// we check both without an else so that both keys down=turn right & walk in place
		if (pinputs[LEFTKEY])
		{
			player->walking = true;
			player->dir = LEFT;
			
			if (player->xinertia > -player->walkspeed)
			{
				player->xinertia -= walk_accel;
				
				if (player->xinertia < -player->walkspeed)
					player->xinertia = -player->walkspeed;
			}
		}
		
		if (pinputs[RIGHTKEY])
		{
			player->walking = true;
			player->dir = RIGHT;
			
			if (player->xinertia < player->walkspeed)
			{
				player->xinertia += walk_accel;
				
				if (player->xinertia > player->walkspeed)
					player->xinertia = player->walkspeed;
			}
		}
		
		if (player->walking && !player->lastwalking)
			player->walkanimframe = 1;
	}
	else
	{
		player->walking = false;
		player->walkanimframe = 0;
		player->walkanimtimer = 0;
		// tap sound when stopped walking
		if (player->lastwalking && player->blockd)
			sound(SND_PLAYER_WALK);
	}
	
	// deceleration
	if (player->blockd && player->yinertia >= 0)
	{	// deceleration on ground...
		// always move towards zero at decelspeed
		if (player->xinertia > 0)
		{
			if (player->blockr && !pinputs[RIGHTKEY])
			{
				player->xinertia = 0;
			}
			else if (player->xinertia > player->decelspeed)
			{
				player->xinertia -= player->decelspeed;
			}
			else
			{
				player->xinertia = 0;
			}
		}
		else if (player->xinertia < 0)
		{
			if (player->blockl && !pinputs[LEFTKEY])
			{
				player->xinertia = 0;
			}
			else if (player->xinertia < -player->decelspeed)
			{
				player->xinertia += player->decelspeed;
			}
			else
			{
				player->xinertia = 0;
			}
		}
	}
	else		// deceleration in air...
	{
		// implements 2 things
		//	1) if player partially hits a brick while in air, his inertia is lesser after he passes it
		//	2) but, if he's trying to turn around, let him! don't "stick" him to it just because
		//		of a high inertia when he hit it
		if (player->blockr)
		{
			limit = (pinputs[RIGHTKEY]) ? 0x180 : 0;
			if (player->xinertia > limit) player->xinertia = limit;
		}
		
		if (player->blockl)
		{
			limit = (pinputs[LEFTKEY]) ? -0x180 : 0;
			if (player->xinertia < limit) player->xinertia = limit;
		}
	}
}

void PDoFalling(void)
{
	if (player->disabled)
		return;
	
	if (player->booststate)
		return;
	
	if (game.curmap == STAGE_KINGS_TABLE && \
		fade.getstate() == FS_FADING)
		return;
	
	// needed to be able to see the falling blocks during
	// good-ending Helicopter cutscene (otherwise your
	// invisible character falls and the blocks spawn too low).
	if (player->hide)
	{
		player->xinertia = 0;
		player->yinertia = 0;
		return;
	}
	
	// use jump gravity as long as Jump Key is down and we're moving up,
	// regardless of whether a jump was ever actually initiated.
	// this is for the fans that blow up--you can push JUMP to climb higher.
	if (player->yinertia < 0 && pinputs[JUMPKEY])
	{	// use jump gravity
		if (player->yinertia < player->fallspeed)
		{
			player->yinertia += player->jumpfallaccel;
			if (player->yinertia > player->fallspeed) player->yinertia = player->fallspeed;
		}
	}
	else
	{	// use normal gravity
		if (player->yinertia < player->fallspeed)
		{
			player->yinertia += player->fallaccel;
			if (player->yinertia > player->fallspeed) player->yinertia = player->fallspeed;
		}
		
		// if we no longer qualify for jump gravity then the jump is over
		player->jumping = 0;
	}
}


void PDoJumping(void)
{
	// jumping
	if (pinputs[JUMPKEY] && !lastpinputs[JUMPKEY])
	{
		if (player->blockd)
		{
			if (!player->jumping)
			{
				player->jumping = true;
				player->yinertia -= player->jumpvelocity;
				sound(SND_PLAYER_JUMP);
			}
		}
		else if ((player->equipmask & (EQUIP_BOOSTER08 | EQUIP_BOOSTER20)))
		{
			PStartBooster();
		}
	}
}


void PDoLooking(void)
{
int lookscroll_want;
int i, key;

	// looking/aiming up and down
	player->look = lookscroll_want = 0;
	
	if (pinputs[DOWNKEY])
	{
		if (!player->blockd)
		{
			player->look = DOWN;
		}
		else if (!lastpinputs[DOWNKEY])
		{	// activating scripts/talking to NPC's
			
			if (!player->walking && !player->lookaway && \
				!pinputs[JUMPKEY] && !pinputs[FIREKEY])
			{
				if (!inputs[DEBUG_MOVE_KEY] || !settings->enable_debug_keys)
				{
					player->lookaway = true;
					player->xinertia = 0;
					PTryActivateScript();
				}
			}
		}
		
		// can still scroll screen down while standing, even though
		// it doesn't show any different frame.
		lookscroll_want = DOWN;
	}
	
	if (pinputs[UPKEY])
	{
		player->look = lookscroll_want = UP;
	}
	
	// when looking, pause a second to be sure they really want to do it
	// before triggering any real screen scrolling
	if (player->lookscroll != lookscroll_want)
	{
		if (player->lookscroll_timer >= 4 || !lookscroll_want)
		{
			player->lookscroll = lookscroll_want;
		}
		else
		{
			player->lookscroll_timer++;
		}
	}
	else
	{
		player->lookscroll_timer = 0;
	}
	
	// deactivation of lookaway
	if (player->lookaway)
	{
		// keys which deactivate lookaway when you are facing away from player
		static const char actionkeys[] = \
			{ LEFTKEY, RIGHTKEY, UPKEY, JUMPKEY, FIREKEY, INPUT_COUNT };
		
		// stop looking away if any keys are pushed
		for(i=0;;i++)
		{
			key = actionkeys[i];
			if (key == INPUT_COUNT) break;
			
			if (pinputs[key])
			{
				player->lookaway = false;
				break;
			}
		}
		
		if (!player->blockd)
			player->lookaway = false;
	}
}

/*
void c------------------------------() {}
*/

// called when the player has just turned on the booster
void PStartBooster(void)
{
	if (player->boosterfuel <= 0)
		return;
	
	// booster 2.0 lets you pick a direction and tacks inertia
	// solid in that direction when first activated
	if ((player->equipmask & EQUIP_BOOSTER20))
	{
		// default boost direction if no key is pressed
		player->booststate = BOOST_UP;
		
		// in order of precedence
		if (inputs[LEFTKEY] || inputs[RIGHTKEY])
			player->booststate = BOOST_HOZ;
		
		if (inputs[DOWNKEY])
			player->booststate = BOOST_DOWN;
		
		if (inputs[UPKEY])
			player->booststate = BOOST_UP;
		
		// set initial inertia full on
		if (player->booststate == BOOST_UP || player->booststate == BOOST_DOWN)
			player->xinertia = 0;
		
		switch(player->booststate)
		{
			case BOOST_UP:
				player->yinertia = -0x5ff;
			break;
			
			case BOOST_DOWN:
				player->yinertia = 0x5ff;
			break;
			
			case BOOST_HOZ:
			{
				player->yinertia = 0;
				
				if (inputs[LEFTKEY])
					player->xinertia = -0x5ff;
				else
					player->xinertia = 0x5ff;
			}
			break;
		}
	}
	else
	{
		player->booststate = BOOST_08;
		
		// help it overcome gravity
		if (player->yinertia > 0x100)
			player->yinertia >>= 1;
	}
	
	PBoosterSmokePuff();
}

// called every tick to run the booster
void PDoBooster(void)
{
	/*static const char *statedesc[] = { "OFF", "UP", "DN", "HOZ", "0.8" };
	debug("fuel: %d", player->boosterfuel);
	debug("booststate: %s", statedesc[player->booststate]);
	debug("xinertia: %d", player->xinertia);
	debug("yinertia: %d", player->yinertia);*/
	
	if (!(player->equipmask & (EQUIP_BOOSTER08 | EQUIP_BOOSTER20)))
	{
		player->booststate = BOOST_OFF;
		return;
	}
	
	if (!pinputs[JUMPKEY])
	{
		player->booststate = BOOST_OFF;
		
		if (player->blockd)
			player->boosterfuel = BOOSTER_FUEL_QTY;
		
		return;
	}
	
	if (!player->booststate)
		return;
	
	// player seems to want it active...check the fuel
	if (player->boosterfuel <= 0)
	{
		player->booststate = BOOST_OFF;
		return;
	}
	else
	{
		player->boosterfuel--;
	}
	
	// ok so then, booster is active right now
	bool sputtering = false;
	
	switch(player->booststate)
	{
		case BOOST_HOZ:
		{
			if ((player->dir == LEFT && player->blockl) || \
				(player->dir == RIGHT && player->blockr))
			{
				player->yinertia = -0x100;
			}
			
			// this probably isn't the right way to do this, but this
			// bit makes the hurt-hop work if you get hit during a sideways boost
			//if (player->hitwhileboosting)
			//	player->yinertia = -0x400;
			
			if (player->dir == LEFT)  player->xinertia -= 0x20;
			if (player->dir == RIGHT) player->xinertia += 0x20;
		}
		break;
		
		case BOOST_UP:
		{
			player->yinertia -= 0x20;
		}
		break;
		
		case BOOST_DOWN:
		{
			player->yinertia += 0x20;
		}
		break;
		
		case BOOST_08:
		{
			// top speed and sputtering
			if (player->yinertia < -0x400)
			{
				player->yinertia += 0x20;
				sputtering = true;	// no sound/smoke this frame
			}
			else
			{
				player->yinertia -= 0x20;
			}
		}
		break;
	}
	
	// don't land if we booster through a one-tile high corridor,
	// but do land if we're, well, landing on something (yinertia not negative).
	// must be done after booster inertia applied to work properly.
	// for 1) there's a place in the village next to Mahin that is good for testing this,
	// for 2) the gaps in outer wall by the little house.
	if (player->blockd)
	{
		if (player->yinertia < 0)
			player->blockd = false;
		else
		{
			player->booststate = BOOST_OFF;
			return;
		}
	}
	
	// smoke and sound effects
	if ((player->boosterfuel % 3) == 1 && !sputtering)
	{
		PBoosterSmokePuff();
	}
}

// called every tick just after PDoBooster returns.
// tones down player's inertia a bit once the Booster 2.0 stops firing
void PDoBoosterEnd()
{
	// put here to be sure it catches all the different ways the Booster can get turned off
	//if (!player->booststate)
		//player->hitwhileboosting = false;
	
	if (player->booststate != player->lastbooststate)
	{
		if (player->booststate == BOOST_OFF && (player->equipmask & EQUIP_BOOSTER20))
		{
			switch(player->lastbooststate)
			{
				case BOOST_HOZ:
					player->xinertia >>= 1;
				break;
				
				case BOOST_UP:
					player->yinertia >>= 1;
				break;
			}
		}
	}
	
	// in the original touching a slope while boosting horizontally
	// disables the booster. In that case, we don't want to half the xinertia,
	// which is why it's here.
	//if (player->booststate == BOOST_HOZ && CheckStandOnSlope(player))
		//player->booststate = BOOST_OFF;
	
	player->lastbooststate = player->booststate;
}

// spawn a Booster smoke puff
void PBoosterSmokePuff()
{
	// these are the directions the SMOKE is traveling, not the player
	//                                 RT   LT    UP    DN
	static const int smoke_xoffs[] = { 10,   4,   7,    7  };
	static const int smoke_yoffs[] = { 10,  10,   0,   14  };
	int smokedir;
	
	switch(player->booststate)
	{
		case BOOST_HOZ: smokedir = (player->dir ^ 1); break;
		case BOOST_UP:	smokedir = DOWN; break;
		case BOOST_DOWN:smokedir = UP; break;
		case BOOST_08:	smokedir = DOWN; break;
		default:		return;
	}
	
	int x = player->x + (smoke_xoffs[smokedir] * CSFI);
	int y = player->y + (smoke_yoffs[smokedir] * CSFI);
	
	Caret *smoke = effect(x, y, EFFECT_SMOKETRAIL_SLOW);
	smoke->MoveAtDir(smokedir, 0x200);
	
	sound(SND_BOOSTER);
}

/*
void c------------------------------() {}
*/

// handle some special characteristics of solid-brick objects,
// such as bouncy and damage. Unlike with FLAG_SOLID_MUSHY; the
// block/l/r/u/d flags for these objects have already been set in
// UpdateBlockStates, so we don't have to worry about those.
void PHandleSolidBrickObjects(void)
{
int i;
SIFSprite *sprite = player->Sprite();
Object *o;

	// calculate total inertia of player--this is needed so that
	// the forcefields in the Monster X arena will damage you if
	// the treads carry you into them.
	int p_xinertia = player->xinertia;
	int p_yinertia = player->yinertia;
	if (player->riding)
	{
		p_xinertia += player->riding->xinertia;
		p_yinertia += player->riding->yinertia;
	}
	
	for(i=0;i<nOnscreenObjects;i++)
	{
		o = onscreen_objects[i];
		if (!(o->flags & FLAG_SOLID_BRICK)) continue;
		
		// left, right, and up contact damage
		if (o->damage > 0)
		{
			if (player->blockl && player->CheckSolidIntersect(o, &sprite->block_l))
			{
				if (p_xinertia < 0 || o->xinertia > 0)
					o->DealContactDamage();
			}
			
			if (player->blockr && player->CheckSolidIntersect(o, &sprite->block_r))
			{
				if (p_xinertia > 0 || o->xinertia < 0)
					o->DealContactDamage();
			}
			
			if (player->blocku && player->CheckSolidIntersect(o, &sprite->block_u))
			{
				if (p_yinertia < 0 || o->yinertia > 0)
					o->DealContactDamage();
			}
		}
		
		// stuff for when you are standing on it
		if (player->blockd && player->CheckSolidIntersect(o, &sprite->block_d))
		{
			if (o->damage && (player->yinertia >= 0 || o->yinertia < 0))
				o->DealContactDamage();
			
			// don't do weird glitchy shit if we jump while being carried upward
			// by an object moving faster than us. handles if you jump while flying
			// momorin's rocket.
			if (player->yinertia < 0 && o->yinertia < player->yinertia)
				player->yinertia = 0;
			
			// handle FLAG_BOUNCY--used eg by treads on Monster X when tipped up
			if (o->flags & FLAG_BOUNCY)
			{
				if (player->yinertia > (o->yinertia - 0x200))
					player->yinertia = (o->yinertia - 0x200);
			}
			else if (o->yinertia <= player->yinertia)
			{
				// snap his Y right on top if it
				player->y = o->SolidTop() - (sprites[player->sprite].block_d[0].y * CSFI);
			}
		}
	}
}


void PHandleSolidMushyObjects(void)
{
	for(int i=0;i<nOnscreenObjects;i++)
	{
		Object *o = onscreen_objects[i];
		
		if (o->flags & FLAG_SOLID_MUSHY)
			PRunSolidMushy(o);
	}
}

// handle "solid mushy" objects, such as bugs. These objects are solid but not 100% super
// solid like a brick. Their solidity is more of an "it repels the player" kind of way.
// NOTE: This is also responsible for the horizontal motion you see when hit by many kinds
// of enemies. The hurtplayer damage routine makes you hop vertically, but it is this that
// throws you away horizontally.
void PRunSolidMushy(Object *o)
{
	// cache these, so we're not calling the same functions over and over again
	const int p_left = player->SolidLeft();
	const int p_right = player->SolidRight();
	const int p_top = player->SolidTop();
	const int p_bottom = player->SolidBottom();
	
	const int o_left = o->SolidLeft();
	const int o_right = o->SolidRight();
	const int o_top = o->SolidTop();
	const int o_bottom = o->SolidBottom();
	
	static const int MUSHY_MARGIN = (3 * CSFI);
	static const int STAND_MARGIN = (1 * CSFI);
	static const int REPEL_FORCE = 0x200;
	
	// hitting sides of object
	if ((p_top < (o_bottom - MUSHY_MARGIN)) && (p_bottom > (o_top + MUSHY_MARGIN)))
	{
		// left side
		if ((p_right > o_left) && (p_right < o->CenterX()))
		{
			if (player->xinertia > -REPEL_FORCE)
				player->xinertia -= REPEL_FORCE;
		}
		
		// right side
		if ((p_left < o_right) && (p_left > o->CenterX()))
		{
			if (player->xinertia < REPEL_FORCE)
				player->xinertia += REPEL_FORCE;
		}
	}
	
	// bonking head on object or standing on object
	
	// to tell if we are within horizontal bounds to be standing on the object,
	// we will check if we have NOT FALLEN OFF the object.
	if (p_left > (o_right - STAND_MARGIN) || p_right < (o_left + STAND_MARGIN))
	{ }
	else
	{
		// standing on object
		if (p_bottom >= o_top && p_bottom <= o->CenterY())
		{
			if (o->flags & FLAG_BOUNCY)
			{
				if (player->yinertia > (o->yinertia - 0x200))
					player->yinertia = (o->yinertia - 0x200);
			}
			else
			{
				// force to top of sprite if we're REALLY far into it
				int em_fline = o->SolidTop() + (3 * CSFI);
				if (player->SolidBottom() > em_fline)
				{
					int over_amt = (em_fline - player->SolidBottom());
					int dec_amt = (3 * CSFI);
					
					if (over_amt < dec_amt) dec_amt = over_amt;
					if (dec_amt < (1 * CSFI)) dec_amt = (1 * CSFI);
					
					player->apply_yinertia(-dec_amt);
				}
				
				player->blockd = true;
				player->riding = o;
			}
		}
		else if (p_top < o_bottom && p_top > o->CenterY())
		{
			// hit bottom of object with head
			if (player->yinertia < 0)
				player->yinertia = 0;
		}
	}
}

/*
void c------------------------------() {}
*/

// does "damage" points of damage to the player
// if even_if_controls_locked is true the damage is
// dealt even if the player's input is locked.
void hurtplayer(int damage)
{
	if (damage == 0) return;
	if (!player || !player->hp) return;
	if (settings->enable_debug_keys && (game.debug.god || inputs[DEBUG_MOVE_KEY])) return;
	
	if (player->hurt_time)
		return;
	
	if (player->hide)
		return;
	
	player->hp -= damage;
	player->DamageText->AddQty(damage);
	
	player->lookaway = 0;
	player->hurt_time = 128;
	
	if (player->equipmask & EQUIP_WHIMSTAR)
		remove_whimstar(&player->whimstar);
	
	//if (player->booststate)
		//player->hitwhileboosting = true;
	
	if (player->hp <= 0)
	{
		sound(SND_PLAYER_DIE);
		SmokeClouds(player, 64, 16, 16);
		rumble(1.0,1000);
		killplayer(SCRIPT_DIED);
	}
	else
	{
		sound(SND_PLAYER_HURT);
		rumble(0.5,500);
		// hop
		if (player->movementmode != MOVEMODE_ZEROG)
			player->yinertia = -0x400;
	}
	
	// decrement weapon XP.
	if (player->equipmask & EQUIP_ARMS_BARRIER)
		SubXP(damage);
	else
		SubXP(damage * 2);
}

// set the player state to "dead" and execute script "script"
void killplayer(int script)
{
	
	player->hp = 0;
	player->dead = true;
	player->hide = true;
	player->xinertia = 0;
	player->yinertia = 0;
	player->riding = NULL;			// why exactly did I say this? i dunno, but not touching for safety
	StopLoopSounds();				// important for Almond
	game.tsc->StartScript(script);
}

/*
void c------------------------------() {}
*/

// this is basically a replacement for most of the player code,
// used when the player is in <UNI0001 (the Ironhead battle).
void PHandleZeroG(void)
{
	if (!player->inputs_locked)
	{
		if (inputs[LEFTKEY] || inputs[RIGHTKEY])
		{
			if (inputs[LEFTKEY])  player->xinertia -= 0x100;
			if (inputs[RIGHTKEY]) player->xinertia += 0x100;
		}
		else
		{	// decel
			if (player->xinertia < 0x80 || player->xinertia > -0x80)
			{
				player->xinertia = 0;
			}
			else
			{
				player->xinertia += (player->xinertia > 0) ? -0x80 : 0x80;
			}
		}
		
		if (inputs[UPKEY] || inputs[DOWNKEY])
		{
			if (inputs[UPKEY]) player->yinertia -= 0x100;
			if (inputs[DOWNKEY]) player->yinertia += 0x100;
		}
		else
		{	// decel
			if (player->yinertia < 0x80 || player->yinertia > -0x80)
			{
				player->yinertia = 0;
			}
			else
			{
				player->xinertia += (player->xinertia > 0) ? -0x80 : 0x80;
			}
		}
	}
	else
	{	// decel for when inputs locked after victory
		if (player->xinertia < 0x80 && player->xinertia > -0x40)
		{
			player->xinertia = 0;
		}
		else
		{
			player->xinertia += (player->xinertia > 0) ? -0x80 : 0x80;
		}
		
		if (player->yinertia < 0x80 && player->yinertia > -0x40)
		{
			player->yinertia = 0;
		}
		else
		{
			player->yinertia += (player->yinertia > 0) ? -0x80 : 0x80;
		}
	}
	
	if (player->xinertia > 0x400)  player->xinertia = 0x400;
	if (player->xinertia < -0x400) player->xinertia = -0x400;
	if (player->yinertia > 0x400)  player->yinertia = 0x400;
	if (player->yinertia < -0x400) player->yinertia = -0x400;
	
	player->frame = (player->yinertia > 0) ? 1 : 2;
}

/*
void c------------------------------() {}
*/

void PInitRepel(void)
{
const int s = SPR_MYCHAR;
int i;

	player->nrepel_l = sprites[s].block_l.count;
	player->nrepel_r = sprites[s].block_r.count;
	player->nrepel_d = sprites[s].block_d.count;
	player->nrepel_u = sprites[s].block_u.count;
	
	for(i=0;i<player->nrepel_l;i++)
	{
		player->repel_l[i].x = sprites[s].block_l[i].x + 1;
		player->repel_l[i].y = sprites[s].block_l[i].y;
	}
	
	for(i=0;i<player->nrepel_r;i++)
	{
		player->repel_r[i].x = sprites[s].block_r[i].x - 1;
		player->repel_r[i].y = sprites[s].block_r[i].y;
	}
	
	for(i=0;i<player->nrepel_d;i++)
	{
		player->repel_d[i].x = sprites[s].block_d[i].x;
		player->repel_d[i].y = sprites[s].block_d[i].y - 1;
	}
	
	for(i=0;i<player->nrepel_u;i++)
	{
		player->repel_u[i].x = sprites[s].block_u[i].x;
		player->repel_u[i].y = sprites[s].block_u[i].y + 1;
	}
}

// the player's block points are assymetrical--block u/d are closer together than block l/r.
// So it's quite possible to get e.g. your blockl points embedded in a wall by
// falling off the top of it. This function implements a SMB1-style "repel" that
// allows this to happen but then pushes the player out of the block over the next
// few frames.
void PDoRepel(void)
{
	// since this function is called from the aftermove, regular player->blockl etc
	// won't be updated until the following frame, so we always check the attributes
	// directly here.
	static const int REPEL_SPEED =	(1 * CSFI);
	
	if (settings->enable_debug_keys && inputs[DEBUG_MOVE_KEY])
		return;
	
	// pushes player out of walls if he become embedded in them, ala Super Mario 1.
	// this can happen for example because his R,L block points are further out than
	// his D block points so it's possible to fall really close to a block and
	// embed the R or L points further into the block than they should be
	if (player->CheckAttribute(player->repel_r, player->nrepel_r, TA_SOLID_PLAYER))
	{
		if (!player->CheckAttribute(&sprites[player->sprite].block_l, TA_SOLID_PLAYER))
		{
			player->x -= REPEL_SPEED;
			//debug("REPEL [to left]");
		}
	}
	
	if (player->CheckAttribute(player->repel_l, player->nrepel_l, TA_SOLID_PLAYER))
	{
		if (!player->CheckAttribute(&sprites[player->sprite].block_r, TA_SOLID_PLAYER))
		{
			player->x += REPEL_SPEED;
			//debug("REPEL [to right]");
		}
	}
	
	// vertical repel doesn't happen normally, but if we get embedded in a
	// block somehow, it can happen.
	/*
	// do repel down
	if (player->CheckAttribute(player->repel_u, player->nrepel_u, TA_SOLID_PLAYER))
	{
		if (!player->CheckAttribute(&sprites[player->sprite].block_d, TA_SOLID_PLAYER))
		{
			player->y += REPEL_SPEED;
			//debug("REPEL [down]");
		}
	}
	*/
	// do repel up
	if (player->CheckAttribute(player->repel_d, player->nrepel_d, TA_SOLID_PLAYER))
	{
		if (!player->CheckAttribute(&sprites[player->sprite].block_u, TA_SOLID_PLAYER))
		{
			player->y -= REPEL_SPEED;
			//debug("REPEL [up]");
		}
	}
	
}

/*
void c------------------------------() {}
*/

static bool RunScriptAtLocation(int x, int y)
{
	// top-to-bottom scan
	for(int i=nOnscreenObjects-1; i>=0; i--)
	{
		Object *o = onscreen_objects[i];
		
		if (o->flags & FLAG_SCRIPTONACTIVATE)
		{
			if (x >= o->Left() && x <= o->Right() && \
				y >= o->Top() && y <= o->Bottom())
			{
				game.tsc->StartScript(o->id2);
				return true;
			}
		}
	}
	
	return false;
}


static bool RunScriptAtX(int x)
{
	if (RunScriptAtLocation(x, player->y + (8 * CSFI)) || \
		RunScriptAtLocation(x, player->y + (14 * CSFI)) || \
		RunScriptAtLocation(x, player->y + (2 * CSFI)))
	{
		return true;
	}
	
	return false;
}



// called when you press down.
// Tries to find an SCRIPTONACTIVATE object you are standing near and activate it.
// if it can't find anything to activate, spawns the "question mark" effect.
void PTryActivateScript()
{
	if (RunScriptAtX(player->CenterX()))
		return;
	
	if (player->dir == RIGHT)
	{
		if (RunScriptAtX(player->Right()) || RunScriptAtX(player->Left()))
			return;
	}
	else
	{
		if (RunScriptAtX(player->Left()) || RunScriptAtX(player->Right()))
			return;
	}
	
	// e.g. Plantation Rocket
	if (player->riding && (player->riding->flags & FLAG_SCRIPTONACTIVATE))
	{
		game.tsc->StartScript(player->riding->id2);
		return;
	}
	
	effect(player->CenterX(), player->CenterY(), EFFECT_QMARK);
}


/*
void c------------------------------() {}
*/

// does the invincibility flash when the player has recently been hurt
void PDoHurtFlash(void)
{
	// note that hurt_flash_state is NOT cleared when timer reaches 0,
	// but this is ok because the number of blinks are and always should be even.
	// (if not it wouldn't look right when he unhurts).
	if (player->hurt_time)
	{
		player->hurt_time--;
		player->hurt_flash_state = (player->hurt_time & 2);
	}
}

// decides which player frame to show
void PSelectFrame(void)
{
	if (player->lookaway)
	{	// looking away
		player->frame = 11;
	}
	else if (!player->blockd || player->yinertia < 0)
	{	// jumping/falling
		player->frame = (player->yinertia > 0) ? 1 : 2;
	}
	else if (player->walking)
	{	// do walk animation
		static const uint8_t pwalkanimframes[] = { 0, 1, 0, 2 };
		
		if (++player->walkanimtimer >= 5)
		{
			player->walkanimtimer = 0;
			if (++player->walkanimframe >= 4) player->walkanimframe = 0;
			if (pwalkanimframes[player->walkanimframe]==0) sound(SND_PLAYER_WALK);
		}
		
		player->frame = pwalkanimframes[player->walkanimframe];
	}
	else
	{	// standing
		player->frame = 0;
	}
	
	// switch frames to "up" or "down" versions if we're looking
	if (player->look)
	{
		if (player->look == UP)
		{
			if (!player->blockd || player->yinertia < 0)
				player->frame = 4;
			else
				player->frame += 3;
		}
		else
		{
			player->frame += 6;
		}
	}
	
	// mimiga mask support-- it would be better to make equipmask private,
	// and funnel all player->equipmask changes through a setter function,
	// then I'd feel safe doing this only when equipped items are changed.
	PSelectSprite();
}

// mimiga mask support
void PSelectSprite(void)
{
	player->sprite = (player->equipmask & EQUIP_MIMIGA_MASK) ? \
					SPR_MYCHAR_MIMIGA : SPR_MYCHAR;
}

/*
void c------------------------------() {}
*/


// returns the sprite and frame # to be used for drawing the given weapon
void GetSpriteForGun(int wpn, int look, int *spr, int *frame)
{
int s;
	
	switch(wpn)
	{
		case WPN_SUPER_MISSILE: s = SPR_SUPER_MLAUNCHER; break;
		case WPN_NEMESIS: s = SPR_NEMESIS; break;
		case WPN_BUBBLER: s = SPR_BUBBLER; break;
		case WPN_SPUR: s = SPR_SPUR; break;
		
		default:
			s = SPR_WEAPONS_START + (wpn * 2);
		break;
	}
	
	if (look)
	{
		s++;
		*frame = (look == DOWN);
	}
	else
	{
		*frame = 0;
	}
	
	*spr = s;
}


// returns the point that a player's shot should be centered on when firing
void GetPlayerShootPoint(int *x_out, int *y_out)
{
int spr, frame;
int x, y;

	GetSpriteForGun(player->curWeapon, player->look, &spr, &frame);
	
	// we have to figure out where the gun is being carried, then figure out where the
	// gun's sprite is drawn relative to that, then finally we can offset in the
	// shoot point of the gun's sprite.
	x = player->x + (sprites[player->sprite].frame[player->frame].dir[player->dir].actionpoint.x * CSFI);
	x -= sprites[spr].frame[frame].dir[player->dir].drawpoint.x * CSFI;
	x += sprites[spr].frame[frame].dir[player->dir].actionpoint.x * CSFI;
	
	y = player->y + (sprites[player->sprite].frame[player->frame].dir[player->dir].actionpoint.y * CSFI);
	y -= sprites[spr].frame[frame].dir[player->dir].drawpoint.y * CSFI;
	y += sprites[spr].frame[frame].dir[player->dir].actionpoint.y * CSFI;
	
	*x_out = x;
	*y_out = y;
}

// draws the player
void DrawPlayer(void)
{
int scr_x, scr_y;

	if (player->hide || player->disabled)
		return;
	
	// keep his floattext position linked--do NOT update this if he is hidden
	// so that floattext doesn't follow him after he dies.
	player->DamageText->UpdatePos(player);
	player->XPText->UpdatePos(player);
	
	// get screen position to draw him at
	scr_x = (player->x / CSFI) - (map.displayed_xscroll / CSFI);
	scr_y = (player->y / CSFI) - (map.displayed_yscroll / CSFI);
	
	// draw his gun
	if (player->curWeapon != WPN_NONE && player->curWeapon != WPN_BLADE)
	{
		int spr, frame;
		GetSpriteForGun(player->curWeapon, player->look, &spr, &frame);
		
		// draw the gun at the player's Action Point. Since guns have their Draw Point set
		// to point at their handle, this places the handle in the player's hand.
		draw_sprite_at_dp(scr_x + sprites[player->sprite].frame[player->frame].dir[player->dir].actionpoint.x, \
						  scr_y + sprites[player->sprite].frame[player->frame].dir[player->dir].actionpoint.y, \
						  spr, frame, player->dir);
	}
	
	// draw the player sprite
	if (!player->hurt_flash_state)
	{
		draw_sprite(scr_x, scr_y, player->sprite, player->frame, player->dir);
		
		// draw the air bubble shield if we have it on
		if (((player->touchattr & TA_WATER) && (player->equipmask & EQUIP_AIRTANK)) || \
			player->movementmode == MOVEMODE_ZEROG)
		{
			draw_sprite_at_dp(scr_x, scr_y, SPR_WATER_SHIELD, \
							  player->water_shield_frame, player->dir);
			
			if (++player->water_shield_timer > 1)
			{
				player->water_shield_frame ^= 1;
				player->water_shield_timer = 0;
			}
		}
	}
	
	if (player->equipmask & EQUIP_WHIMSTAR)
		draw_whimstars(&player->whimstar);
}


