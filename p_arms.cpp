
#include "nx.h"
#include "p_arms.h"
#include "ObjManager.h"
#include "caret.h"
#include "player.h"
#include "playerstats.h"
#include "statusbar.h"
#include "input.h"
#include "ai/weapons/whimstar.h"
#include "sound/sound.h"
#include "common/misc.h"

#include "autogen/sprites.h"
#include "game.h"
#include "console.h"

static Object *FireSimpleBullet(int otype, int btype, int xoff=0, int yoff=0);
static int empty_timer = 0;

struct BulletInfo
{
	int sprite;				// sprite to use
	int level;				// specify what level weapon is at when it fires this shot type
	int frame;				// specify which frame within sprite
	uint8_t makes_star;		// 1=make star effect, 2=make star but add x inertia to position
	int timetolive;			// shot range
	int damage;				// damage dealt per tick of contact with enemy
	int speed;				// speed of shot
	uint8_t manualsetup;	// 1= no auto setup at all, 2= don't use separate vert sprite
	uint8_t sound;			// specify firing sound
};

BulletInfo bullet_table[] =
{
//		sprite			  lvl  frm st ttl dmg spd  manset      sound
	SPR_SHOT_POLARSTAR, 	0,  0, 1, 8,   1, 0x1000, 0, SND_POLAR_STAR_L1_2,		// polarstar l1
	SPR_SHOT_POLARSTAR, 	1,  1, 1, 12,  2, 0x1000, 0, SND_POLAR_STAR_L1_2,		// polarstar l2
	SPR_SHOT_POLARSTAR_L3, 	2,  0, 1, 16,  4, 0x1000, 0, SND_POLAR_STAR_L3,		// polarstar l3
	
	SPR_SHOT_MGUN_L1, 		0,  0, 1, 20,  2, 0x1000, 0, SND_POLAR_STAR_L1_2,		// mgun l1
	
	SPR_SHOT_MGUN_L2,		1,  0, 1, 20,  4, 0x1000, 0, SND_POLAR_STAR_L1_2,		// mgun l2, white piece
	SPR_SHOT_MGUN_L2,		1,  1, 0, 21,  0, 0x1000, 0, 0,						// mgun l2, blue piece
	SPR_SHOT_MGUN_L2,		1,  2, 0, 22,  0, 0x1000, 0, 0,						// mgun l2, dark piece
	
	SPR_SHOT_MGUN_L3LEAD,	2,  0, 1, 20,  6, 0x1000, 0, SND_POLAR_STAR_L3,		// mgun l3
	SPR_SHOT_MGUN_L3TAIL,	2,  0, 0, 21,  0, 0x1000, 0, 0,						// the very long...
	SPR_SHOT_MGUN_L3TAIL,	2,  1, 0, 22,  0, 0x1000, 0, 0,						// ...4 piece trail...
	SPR_SHOT_MGUN_L3TAIL,	2,  2, 0, 23,  0, 0x1000, 0, 0,						// ...of the level 3...
	SPR_SHOT_MGUN_L3TAIL,	2,  3, 0, 24,  0, 0x1000, 0, 0,						// ...machine gun
	
	// damage for missiles is set inside missile.cpp
	SPR_SHOT_MISSILE1,		0,  0, 1, 50,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// missile level 1
	SPR_SHOT_MISSILE2,		1,  0, 1, 65,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// missile level 2
	SPR_SHOT_MISSILE3,		2,  0, 1, 90,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// missile level 3
	
	SPR_SHOT_SUPERMISSILE13,0,  0, 1, 30,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// supermissile l1
	SPR_SHOT_SUPERMISSILE2,	1,  0, 1, 40,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// supermissile l2
	SPR_SHOT_SUPERMISSILE13,2,  0, 1, 40,  0, 0x0000, 0, SND_POLAR_STAR_L1_2,	// supermissile l3
	
	// damages are doubled because fireball can hit twice before dissipating
	SPR_SHOT_FIREBALL1,		0,  0, 1, 100, 2, 0x0000, 1, SND_FIREBALL,		// fireball l1
	SPR_SHOT_FIREBALL23,	1,  0, 1, 100, 3, 0x0000, 1, SND_FIREBALL,		// fireball l2
	SPR_SHOT_FIREBALL23,	2,  0, 1, 100, 3, 0x0000, 1, SND_FIREBALL,		// fireball l3
	
	SPR_SHOT_BLADE_L1,		0,  0, 0, 29, 15, 0x800,  0, SND_FIREBALL,		// Blade L1
	SPR_SHOT_BLADE_L2,		1,  0, 0, 17, 6,  0x800,  0, SND_FIREBALL,		// Blade L2
	SPR_SHOT_BLADE_L3,		2,  0, 0, 30, 1,  0x800,  0, SND_FIREBALL,		// Blade L3
	
	SPR_SHOT_SNAKE_L1,		0,  0, 1, 20, 4,  0x600,  2, SND_SNAKE_FIRE,	// Snake L1
	SPR_SHOT_FIREBALL23,	1,	0, 1, 23, 6,  0x200,  2, SND_SNAKE_FIRE,	// Snake L2
	SPR_SHOT_FIREBALL23,	2,	0, 1, 30, 8,  0x200,  2, SND_SNAKE_FIRE,	// Snake L3
	
	SPR_SHOT_NEMESIS_L1,	0,  0, 2, 20, 12, 0x1000, 0, SND_NEMESIS_FIRE,
	SPR_SHOT_NEMESIS_L2,	1,  0, 2, 20, 6,  0x1000, 0, SND_POLAR_STAR_L3,
	SPR_SHOT_NEMESIS_L3,	2,  0, 2, 20, 1,  0x555,  0, 0,		// 1/3 speed
	
	SPR_SHOT_BUBBLER_L1,	0,	0, 1, 40, 1,  0x600,  2, SND_BUBBLER_FIRE,
	SPR_SHOT_BUBBLER_L2,	1,	0, 1, 60, 2,  0x600,  2, SND_BUBBLER_FIRE,
	SPR_SHOT_BUBBLER_L3,	2,	0, 1, 100,2,  0x600,  2, SND_BUBBLER_FIRE,
	
	// Spur also messes with it's damage at runtime; see spur.cpp for details.
	SPR_SHOT_POLARSTAR,		0,	0, 1, 30, 4,  0x1000, 0, SND_SPUR_FIRE_1,
	SPR_SHOT_POLARSTAR,		1,	1, 1, 30, 8,  0x1000, 0, SND_SPUR_FIRE_2,
	SPR_SHOT_POLARSTAR_L3,	2,  0, 0, 30, 12, 0x1000, 0, SND_SPUR_FIRE_3,
	
	// Curly's Nemesis from Hell (OBJ_CURLY_CARRIED_SHOOTING)
	SPR_SHOT_NEMESIS_L1,	0,  0, 1, 20, 12, 0x1000, 0, SND_NEMESIS_FIRE,
	
	0, 0, 0, 0, 0, 0, 0
};


// resets anything like charging states etc on player re-init (Player::Init)
void PResetWeapons()
{
	Weapon *spur = &player->weapons[WPN_SPUR];
	spur->chargetimer = 0;
	spur->level = 0;
	spur->xp = 0;
	
	init_whimstar(&player->whimstar);
}


void PDoWeapons(void)
{
	// switching weapons. have to check for inputs_frozen since justpushed
	// reads inputs[] directly, not pinputs[].
	if (!player->inputs_locked)
	{
		if (justpushed(PREVWPNKEY)) stat_PrevWeapon();
		if (justpushed(NEXTWPNKEY)) stat_NextWeapon();
	}
	
	// firing weapon
	if (pinputs[FIREKEY])
	{
		FireWeapon();
		RunWeapon(true);
	}
	else
	{
		RunWeapon(false);
	}
	
	PHandleSpur();
	run_whimstar(&player->whimstar);
	
	if (empty_timer)
		empty_timer--;
}

/*
void c------------------------------() {}
*/

// called when player is trying to fire the current weapon
// i.e. the fire button is down.
void FireWeapon(void)
{
Weapon *curweapon = &player->weapons[player->curWeapon];
int level = curweapon->level;

	// check if we can fire
	if (curweapon->firerate[level] != 0)
	{	// rapid/fully-auto fire
		// decremented in RunWeapon()
		if (curweapon->firetimer)
		{
			return;
		}
		else
		{
			curweapon->firetimer = curweapon->firerate[level];
		}
	}
	else
	{	// else must push key for each shot
		if (lastpinputs[FIREKEY])
			return;
	}
	
	// check if we have enough ammo
	if (curweapon->maxammo > 0 && curweapon->ammo <= 0)
	{
		sound(SND_GUN_CLICK);
		if (empty_timer <= 0)
		{
			effect(player->CenterX(), player->CenterY(), EFFECT_EMPTY);
			empty_timer = 50;
		}
		
		return;
	}
	
	// subtract ammo
	if (curweapon->ammo)
		curweapon->ammo--;
	
	// fire!!
	switch(player->curWeapon)
	{
		case WPN_NONE: break;
		
		case WPN_POLARSTAR:
			PFirePolarStar(level);
		break;
		
		case WPN_FIREBALL:
			PFireFireball(level);
		break;
		
		case WPN_MGUN:
			PFireMachineGun(level);
		break;
		
		case WPN_MISSILE:
		case WPN_SUPER_MISSILE:
			PFireMissile(level, (player->curWeapon == WPN_SUPER_MISSILE));
		break;
		
		case WPN_BLADE:
			PFireBlade(level);
		break;
		
		case WPN_SNAKE:
			PFireSnake(level);
		break;
		
		case WPN_NEMESIS:
			PFireNemesis(level);
		break;
		
		case WPN_BUBBLER:
			PFireBubbler(level);
		break;
		
		case WPN_SPUR:
			PFireSpur();
		break;
		
		default:
			console.Print("FireWeapon: cannot fire unimplemented weapon %d", player->curWeapon);
			sound(SND_BONK_HEAD);
		break;
	}
}


// "run" the current weapon.
// firing = 1 if fire key is currently down, and 0 if it is not.
void RunWeapon(bool firing)
{
Weapon *curweapon = &player->weapons[player->curWeapon];
int level = curweapon->level;

	// bubbler L1 has recharge but not rapid fire,
	// so it recharges even if the key is held down.
	if (firing && !curweapon->firerate[level] && lastpinputs[FIREKEY])
		firing = false;
	
	// recharge machine gun when it's not firing or it's not selected
	if ((curweapon->rechargerate[level]) && \
		(curweapon->ammo < curweapon->maxammo) && \
		!firing)
	{
		// start recharging ammo
		int rate = curweapon->rechargerate[level];
		if ((player->equipmask & EQUIP_TURBOCHARGE) && player->curWeapon == WPN_MGUN)
		{
			rate = 2;
		}
		
		// it's greater than OR EQUAL TO, so that we can have rate=0 be no recharge.
		// Otherwise there would be no value that recharges every frame.
		if (++curweapon->rechargetimer >= rate)
		{
			curweapon->rechargetimer = 0;
			curweapon->ammo++;
		}
	}
	
	for(int i=0;i<WPN_COUNT;i++)
	{
		if (player->weapons[i].firetimer)
			player->weapons[i].firetimer--;
		
		if ((i != player->curWeapon) || \
			(player->weapons[i].ammo >= player->weapons[i].maxammo) || \
			firing)
		{
			player->weapons[i].rechargetimer = 0;
		}
	}
}

/*
void c------------------------------() {}
*/

// set up the specified bullet to be a shot of type btype
// (note: shared by Curly sand-zone boss)
void SetupBullet(Object *shot, int x, int y, int btype, int dir)
{
	const BulletInfo *info = &bullet_table[btype];
	
	shot->sprite = info->sprite;
	shot->frame = info->frame;
	shot->shot.ttl = info->timetolive;
	shot->shot.damage = info->damage;
	shot->shot.level = info->level;
	shot->shot.btype = btype;
	shot->shot.dir = dir;
	shot->nxflags |= NXFLAG_NO_RESET_YINERTIA;
	
	if (game.debug.infinite_damage)
		shot->shot.damage = 255;
	
	if (info->sound)
		sound(info->sound);
	
	if (info->makes_star == 1)
		effect(x, y, EFFECT_STARPOOF);
	
	if (info->manualsetup != 1)
	{
		switch(dir)
		{
			case LEFT:
				shot->xinertia = -info->speed;
				shot->dir = LEFT;
			break;
			
			case RIGHT:
				shot->xinertia = info->speed;
				shot->dir = RIGHT;
			break;
			
			case UP:
				shot->yinertia = -info->speed;
				shot->dir = RIGHT;
				if (info->manualsetup != 2) { shot->sprite++; }
			break;
			
			case DOWN:
				shot->yinertia = info->speed;
				shot->dir = LEFT;
				if (info->manualsetup != 2) { shot->sprite++; }
			break;
		}
		
		if (info->makes_star == 2)
			effect(x+shot->xinertia/2, y, EFFECT_STARPOOF);
		
		// have to do this because inertia will get applied later in the tick before the first
		// time it's drawn so it won't actually appear where we put it if we don't
		x -= shot->xinertia;
		y -= shot->yinertia;
	}
	
	// put shot center at [x,y],
	// this also centers it within starpoof
	shot->x = x - (shot->Width() / 2);
	shot->y = y - (shot->Height() / 2);
}


// fire a basic, single bullet
static Object *FireSimpleBullet(int otype, int btype, int xoff, int yoff)
{
int x, y, dir;

	// get location to fire from
	GetPlayerShootPoint(&x, &y);
	x += xoff;
	y += yoff;
	
	// create the shot
	Object *shot = CreateObject(0, 0, otype);
	
	// set up the shot
	if (player->look)
		dir = player->look;
	else
		dir = player->dir;
	
	SetupBullet(shot, x, y, btype, dir);
	return shot;
}

// fires a bullet at an offset from the exact center of the player's shoot point.
// FireSimpleBullet can do this too-- but it's xoff/yoff is absolute. This function
// takes a parameter for when you are shooting right and extrapolates out the other
// directions from that. ALSO, xoff/yoff on FireSimpleBullet moves the star;
// this function does not.
static Object *FireSimpleBulletOffset(int otype, int btype, int xoff, int yoff)
{
int dir;

	if (player->look)
		dir = player->look;
	else
		dir = player->dir;
	
	switch(dir)
	{
		case RIGHT: break;	// already in format for RIGHT frame
		case LEFT: xoff = -xoff; break;
		case UP: SWAP(xoff, yoff); yoff = -yoff; break;
		case DOWN: SWAP(xoff, yoff); break;
	}
	
	Object *shot = FireSimpleBullet(otype, btype);
	shot->x += xoff;
	shot->y += yoff;
	
	return shot;
}


/*
void c------------------------------() {}
*/

static void PFirePolarStar(int level)
{
	// at level 3 only two shots per screen permitted
	if (level < 2 || CountObjectsOfType(OBJ_POLAR_SHOT) < 2)
	{
		int xoff;
		if (level == 2) xoff = -5<<CSF; else xoff = -4<<CSF;
		
		FireSimpleBulletOffset(OBJ_POLAR_SHOT, B_PSTAR_L1+level, xoff, 0);
	}
}

/*
void c------------------------------() {}
*/

// handles firing the Machine Gun
static void PFireMachineGun(int level)
{
Object *shot;
int x, y;

	int dir = (player->look) ? player->look : player->dir;
	
	if (level == 0)
	{	// level 1 is real easy! no frickin' layers!!
		shot = FireSimpleBullet(OBJ_POLAR_SHOT, B_MGUN_L1, 0, 0);
		shot->dir = dir;
		
		if (player->look)
			shot->xinertia = random(-0xAA, 0xAA);
		else
			shot->yinertia = random(-0xAA, 0xAA);
	}
	else
	{
		// drop an OBJ_MGUN_SHOOTER object to fire the layers (trail) of the MGun blast.
		GetPlayerShootPoint(&x, &y);
		FireLevel23MGun(x, y, level, dir);
	}
	
	// do machine-gun flying
	if (player->look==DOWN && level==2)
	{
		PMgunFly();
	}
}

// fire a level 2 or level 3 MGun blast from position x,y.
// Broken out here into a seperate sub so OBJ_CURLY_AI can use it also.
void FireLevel23MGun(int x, int y, int level, int dir)
{
static const uint8_t no_layers[] = { 1, 3, 5 };
static const int bultype_table[] = { 0, B_MGUN_L2, B_MGUN_L3 };
Object *shot;

	// note: this relies on the player AI running before the entity AI...which it does...
	// so leave it that way, else he wouldn't actually fire for 1 additional frame
	shot = CreateObject(x, y, OBJ_MGUN_SPAWNER);
	
	shot->dir = dir;
	shot->mgun.bultype = bultype_table[level];
	shot->mgun.nlayers = no_layers[level];
	shot->mgun.wave_amt = random(-0xAA, 0xAA);
	shot->invisible = true;
}


// handles flying when shooting down using Machine Gun at Level 3
void PMgunFly(void)
{
	if (player->yinertia > 0)
	{
		player->yinertia >>= 1;
	}
	
	if (player->yinertia > -0x400)
	{
		player->yinertia -= 0x200;
		if (player->yinertia < -0x400) player->yinertia = -0x400;
	}
}

/*
void c------------------------------() {}
*/

// fire the missile launcher.
// level: 0 - 2: weapon level from 1 - 3
// is_super: bool: true if the player is firing the Super Missile Launcher
static void PFireMissile(int level, bool is_super)
{
Object *o;
int xoff, yoff;

	int object_type = (!is_super) ? OBJ_MISSILE_SHOT : OBJ_SUPERMISSILE_SHOT;
	
	// can only fire one missile at once on L1,
	// two missiles on L2, and two sets of three missiles on L3.
	static const uint8_t max_missiles_at_once[] = { 1, 2, 6 };
	if (CountObjectsOfType(object_type) >= max_missiles_at_once[level])
	{
		// give back the previously-decremented ammo so they don't lose it (hack)
		player->weapons[player->curWeapon].ammo++;
		return;
	}
	
	int bullet_type = (!is_super) ? B_MISSILE_L1 : B_SUPER_MISSILE_L1;
	bullet_type += level;
	
	// level 1 & 2 fires just one missile
	FireSimpleBulletOffset(object_type, bullet_type, -4<<CSF, 0);
	
	// level 3 fires three missiles, they wave, and are "offset",
	// so if it's level 3 fire two more missiles.
	if (level == 2)
	{
		//									 norm	 super
		static const int recoil_upper[] = { 0x500,  0xd00 };
		static const int recoil_lower[] = { 0x700,  0x600 };
		
		if (player->look==DOWN || player->look==UP) { xoff = (4<<CSF); yoff = 0; }
											   else { yoff = (4<<CSF); xoff = 0; }
		
		// this one is higher
		o = FireSimpleBullet(object_type, bullet_type, -xoff, -yoff);
		if (o->shot.dir==LEFT) 		 o->xinertia = recoil_upper[is_super];
		else if (o->shot.dir==RIGHT) o->xinertia = -recoil_upper[is_super];
		else if (o->shot.dir==UP) 	 o->yinertia = recoil_upper[is_super];
		else 						 o->yinertia = -recoil_upper[is_super];
		
		// this one is lower
		o = FireSimpleBullet(object_type, bullet_type, xoff, yoff);
		if (o->shot.dir==LEFT) 		 o->xinertia = recoil_lower[is_super];
		else if (o->shot.dir==RIGHT) o->xinertia = -recoil_lower[is_super];
		else if (o->shot.dir==UP) 	 o->yinertia = recoil_lower[is_super];
		else 						 o->yinertia = -recoil_lower[is_super];
	}
}

/*
void c------------------------------() {}
*/

static void PFireFireball(int level)
{
static const int object_types[] = { OBJ_FIREBALL1, OBJ_FIREBALL23, OBJ_FIREBALL23 };
static uint8_t max_fireballs[] = { 2, 3, 4 };
int count;

	count = (CountObjectsOfType(OBJ_FIREBALL1) + CountObjectsOfType(OBJ_FIREBALL23));
	if (count >= max_fireballs[level])
	{
		return;
	}
	
	// the 8px offset fires the shot just a tiny bit behind the player--
	// you can't see the difference but it makes the shot correctly bounce if
	// you shoot while flat up against a wall, instead of embedding the fireball
	// in the wall.
	Object *fb = FireSimpleBulletOffset(object_types[level], B_FIREBALL1 + level, -8<<CSF, 0);
	fb->dir = player->dir;
	fb->nxflags &= ~NXFLAG_NO_RESET_YINERTIA;
	
	switch(fb->shot.dir)
	{
		case LEFT: fb->xinertia = -0x400; break;
		case RIGHT: fb->xinertia = 0x400; break;
		
		case UP:
			fb->xinertia = player->xinertia + ((player->dir==RIGHT) ? 128 : -128);
			if (player->xinertia) fb->dir = (player->xinertia > 0) ? RIGHT:LEFT;
			fb->yinertia = -0x5ff;
		break;
		
		case DOWN:
			fb->xinertia = player->xinertia;
			if (player->xinertia) fb->dir = (player->xinertia > 0) ? RIGHT:LEFT;
			fb->yinertia = 0x5ff;
		break;
	}

}

static void PFireBlade(int level)
{
	int numblades = CountObjectsOfType(OBJ_BLADE12_SHOT) + CountObjectsOfType(OBJ_BLADE3_SHOT);
	if (numblades >= 1) return;
	
	int dir = (player->look) ? player->look : player->dir;
	
	int x = player->CenterX();
	int y = player->CenterY();
	
	if (level == 2)
	{
		if (dir == RIGHT || dir == LEFT)
		{
			y -= (3 << CSF);
			x += (dir == LEFT) ? (3 << CSF) : -(3 << CSF);
		}
	}
	else
	{
		switch(dir)
		{
			case RIGHT: x -= (6 << CSF); y -= (3 << CSF); break;
			case LEFT:  x += (6 << CSF); y -= (3 << CSF); break;
			case UP:    y += (6 << CSF); break;
			case DOWN:  y -= (6 << CSF); break;
		}
	}
	
	Object *shot = CreateObject(x, y, (level != 2) ? OBJ_BLADE12_SHOT : OBJ_BLADE3_SHOT);
	SetupBullet(shot, x, y, B_BLADE_L1+level, dir);
}

/*
void c------------------------------() {}
*/

static void PFireSnake(int level)
{
	if (level == 2)
	{
		int count = (CountObjectsOfType(OBJ_SNAKE1_SHOT) + \
					 CountObjectsOfType(OBJ_SNAKE23_SHOT));
		
		if (count >= 4)
			return;
	}
	
	int object_type = (level == 0) ? OBJ_SNAKE1_SHOT : OBJ_SNAKE23_SHOT;
	FireSimpleBulletOffset(object_type, B_SNAKE_L1+level, -5<<CSF, 0);
}


static void PFireNemesis(int level)
{
	if (CountObjectsOfType(OBJ_NEMESIS_SHOT) >= 2)
		return;
	
	FireSimpleBullet(OBJ_NEMESIS_SHOT, B_NEMESIS_L1+level);
}


static void PFireBubbler(int level)
{
static const int max_bubbles[] = { 4, 16, 16 };

	int count = CountObjectsOfType(OBJ_BUBBLER12_SHOT) + \
				CountObjectsOfType(OBJ_BUBBLER3_SHOT);
	
	if (count >= max_bubbles[level])
		return;
	
	int objtype = (level != 2) ? OBJ_BUBBLER12_SHOT : OBJ_BUBBLER3_SHOT;
	FireSimpleBulletOffset(objtype, B_BUBBLER_L1+level, -4<<CSF, 0);
}

/*
void c------------------------------() {}
*/

// Spur fires an initial shot of Polar Star L3, then charges
// as long as key is down. Fires when key released.
// Released at L1: nothing
// Released at L2: thin beam
// Released at L3: dual beam
// Released at Max: thick beam
//
// Initial shot is not fired if key is held on a different weapon
// and then weapon is switched to spur.

// fires the regular Polar Star shot when you first push button
static void PFireSpur(void)
{
	if (can_fire_spur())
		FireSimpleBulletOffset(OBJ_POLAR_SHOT, B_PSTAR_L3, -4<<CSF, 0);
}

// fires and handles charged shots
static void PHandleSpur(void)
{
static const int FLASH_TIME = 10;
Weapon *spur = &player->weapons[WPN_SPUR];

	if (player->curWeapon != WPN_SPUR)
	{
		spur->level = 0;
		spur->xp = 0;
		return;
	}
	
	if (pinputs[FIREKEY])
	{
		if (!IsWeaponMaxed())
		{
			int amt = (player->equipmask & EQUIP_TURBOCHARGE) ? 3 : 2;
			AddXP(amt, true);
			
			if (IsWeaponMaxed())
			{
				sound(SND_SPUR_MAXED);
			}
			else
			{
				spur->chargetimer++;
				if (spur->chargetimer & 2)
				{
					sound(SND_SPUR_CHARGE_1 + spur->level);
				}
			}
		}
		else
		{	// keep flashing even once at max
			statusbar.xpflashcount = FLASH_TIME;
			
			if (player->equipmask & EQUIP_WHIMSTAR)
				add_whimstar(&player->whimstar);
		}
	}
	else
	{
		if (spur->chargetimer)
		{
			if (spur->level > 0 && can_fire_spur())
			{
				int level = IsWeaponMaxed() ? 2 : (spur->level - 1);
				FireSimpleBulletOffset(OBJ_SPUR_SHOT, B_SPUR_L1+level, -4<<CSF, 0);
			}
			
			spur->chargetimer = 0;
		}
		
		spur->level = 0;
		spur->xp = 0;
	}
	
	if (statusbar.xpflashcount > FLASH_TIME)
		statusbar.xpflashcount = FLASH_TIME;

}

static bool can_fire_spur(void)
{
	if (CountObjectsOfType(OBJ_SPUR_SHOT))
		return false;
	
	return true;
}

// returns true if the current weapon has full xp at level 3 (is showing "Max")
static bool IsWeaponMaxed(void)
{
	Weapon *wpn = &player->weapons[player->curWeapon];
	return (wpn->level == 2) && (wpn->xp == wpn->max_xp[2]);
}















