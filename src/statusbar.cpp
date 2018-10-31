
#include "nx.h"
#include "statusbar.h"
#include "graphics/sprites.h"
#include "graphics/graphics.h"
using namespace Graphics;
#include "autogen/sprites.h"
#include "screeneffect.h"
using namespace Sprites;
#include "sound/SoundManager.h"
#include "player.h"
#include "game.h"

#ifdef WIDESCREEN
	#define STATUS_X	10
	#define STATUS_Y	10
#else
	#define STATUS_X	16
	#define STATUS_Y	16
#endif

#define HEALTH_X			(STATUS_X+0)
#define HEALTH_Y			(STATUS_Y+24)
#define HEALTHFILL_X		(HEALTH_X+24)
#define HEALTHFILL_Y		(HEALTH_Y+1)
#define HEALTHFILL_MAXLEN	39

#define WEAPONBAR_Y			(STATUS_Y+1)
#define CURWEAPON_X			(STATUS_X+1)

#define XPBAR_Y				(STATUS_Y+16)		// Y coordinate of "XP" area
#define XPBAR_X				(STATUS_X+24)		// X of yellow XP bar

#define AMMO_X				(STATUS_X+32)
#define AMMO_Y				(STATUS_Y+0)

#define NIKU_X				STATUS_X
#define NIKU_Y				8

#define FRAME_XP_BAR		0			// empty bar frame
#define FRAME_XP_FILL		1			// sprite to fill bar with
#define FRAME_XP_FLASH		2			// white flashing when XP is gained
#define FRAME_XP_MAX		3			// "MAX" when XP is at max on L3

//int displayed_health = 0;
//int healthdectimer = 0;
StatusBar statusbar;
static PercentBar PHealthBar;

// the "slide" effect when changing weapons
struct stWeaponSlide
{
	int lv_offset;					// offset of XP bar
	int wpn_offset;					// offset of weapon bar
	int ammo_offset;				// offset of ammo, in addition to wpn_offset
	char timer;
	int move_dir;
	int firstWeapon;				// weapon to show as current weapon
} slide;
#define SLIDE_LV_OFFSET			16
#define SLIDE_TIMER_START		5


bool statusbar_init(void)
{
	InitPercentBar(&PHealthBar, player->hp);
	
	memset(&slide, 0, sizeof(slide));
	slide.firstWeapon = player->curWeapon;
	return 0;
}



void DrawAirLeft(int x, int y)
{
	if (player->airshowtimer)
	{
		draw_sprite(x, y, SPR_AIR, (player->airleft%30 > 10) ? 0:1, RIGHT);
		
		if (player->airshowtimer%6 < 4)
			DrawNumber(x+32, y, player->airleft/10);
	}
}

void DrawWeaponAmmo(int x, int y, int wpn)
{
	// draw slash
	if (!player->hurt_flash_state || game.mode != GM_NORMAL)
	{
		draw_sprite(x, y+8, SPR_WHITENUMBERS, 11, 0);
	}
	
	if (!player->weapons[wpn].maxammo)
	{	// ammo is "not applicable"
		x += 16;
		draw_sprite(x, y, SPR_NAAMMO, 0, 0);
		draw_sprite(x, y+8, SPR_NAAMMO, 0, 0);
	}
	else
	{
		DrawNumber(x, y, player->weapons[wpn].ammo);
		DrawNumber(x, y+8, player->weapons[wpn].maxammo);
	}
}

void DrawWeaponLevel(int x, int y, int wpn)
{
	int level = (player->weapons[wpn].level + 1);
	if (wpn == WPN_NONE) level = 0;
	
	draw_sprite(x, y, SPR_XPLEVELICON, 0, 0);
	draw_sprite(x+16, y, SPR_WHITENUMBERS, level, 0);
}


static void RunStatusBar(void)
{
	// handle slowly decreasing the health when player is hurt
	// note how it only decrements while it's actually visible--i thought that was a nice touch
	if (!player->hurt_flash_state)
	{
		RunPercentBar(&PHealthBar, player->hp);
	}
	
	if (game.frozen || player->inputs_locked) return;
	if (fade.getstate() != FS_NO_FADE) return;
	
	// sliding effect when changing weapons
	if (slide.lv_offset)
	{	// next weapon
		slide.lv_offset += slide.move_dir;
		if (--slide.timer > 0)
		{
			slide.wpn_offset += slide.move_dir;
		}
		else
		{
			if (!slide.timer) slide.firstWeapon = player->curWeapon;
			slide.wpn_offset = slide.lv_offset;
			slide.ammo_offset = 0;
		}
	}
	else
	{
		slide.firstWeapon = player->curWeapon;
	}
}


// start the slide effect. if dir = LEFT, slides left (next weapon), if RIGHT does "prev weapon"
// newwpn = the weapon to change to
void weapon_slide(int dir, int newwpn)
{
int sign;
	if (slide.lv_offset) slide.firstWeapon = player->curWeapon;		// if already sliding change immediately
	if (dir==RIGHT) sign = -1; else sign = 1;
	
	slide.lv_offset = SLIDE_LV_OFFSET * sign;
	slide.timer = SLIDE_TIMER_START;
	slide.ammo_offset = 16 * sign;
	slide.move_dir = -2 * sign;
	player->curWeapon = newwpn;
}

// the opening slide effect on load/new game
void weapon_introslide()
{
	if (player->curWeapon == WPN_NONE)
	{
		weapon_slide(LEFT, player->curWeapon);
		return;
	}
	
	stat_PrevWeapon(true);
	slide.lv_offset = 0;
	stat_NextWeapon(true);
}

// switches to the next weapon in inventory
void stat_NextWeapon(bool quiet)
{
int w;

	w = player->curWeapon;
	if (w == WPN_NONE) return;
	
	for(;;)
	{
		if (++w >= WPN_COUNT) w = 0;
		
		if (player->weapons[w].hasWeapon || w == player->curWeapon)
		{
			if (!quiet)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SWITCH_WEAPON);
			weapon_slide(LEFT, w);
			return;
		}
	} while(w != player->curWeapon);
}

// switches to the previous weapon in inventory
void stat_PrevWeapon(bool quiet)
{
int w;

	w = player->curWeapon;
	if (w == WPN_NONE) return;
	
	for(;;)
	{
		if (--w < 0) w = WPN_COUNT-1;
		
		if (player->weapons[w].hasWeapon || w == player->curWeapon)
		{
			if (!quiet)
				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SWITCH_WEAPON);
			weapon_slide(RIGHT, w);
			return;
		}
	}
}


void InitPercentBar(PercentBar *bar, int starting_value)
{
	PHealthBar.displayed_value = starting_value;
	PHealthBar.dectimer = 0;
}

void RunPercentBar(PercentBar *bar, int current_value)
{
	if (current_value != bar->displayed_value)
	{
		if (current_value > bar->displayed_value)
		{
			bar->displayed_value = current_value;
		}
		else
		{
			if (++bar->dectimer > 0x1e)
			{
				bar->displayed_value--;
			}
		}
	}
	else bar->dectimer = 0;
}

void DrawPercentBar(PercentBar *bar, int x, int y, int curvalue, int maxvalue, int width)
{
	if (bar->displayed_value != curvalue)
		DrawPercentage(x, y, SPR_HEALTHFILL, 1, bar->displayed_value, maxvalue, width);
		
	DrawPercentage(x, y, SPR_HEALTHFILL, 0, curvalue, maxvalue, width);
}

/*
void c------------------------------() {}
*/

// draws number "num" at x,y.
// leading zeroes are omitted, however, the space for them is still
// skipped over (left blank). thus it always leaves space for 3 digits.
void DrawNumber(int x, int y, int num)
{
static const int numtable[3] = { 1000, 100, 10 };
int place, digit, total;

	if (num > 9999) num = 9999;
	
	place = total = 0;
	while(place < 3)
	{
		digit = 0;
		while(num >= numtable[place])
		{
			num -= numtable[place];
			digit++;
		}
		
		total += digit;
		
		if (total)
			draw_sprite(x+(place*8), y, SPR_WHITENUMBERS, digit);
		
		place++;
	}
	
	draw_sprite(x+(3*8), y, SPR_WHITENUMBERS, num);
}


void DrawPercentage(int x, int y, int fill_sprite, int fsframe, int curvalue, int maxvalue, int width_at_max)
{
	if (curvalue < 0) curvalue = 0;
	if (curvalue > 0 || maxvalue==0)
	{
		int fillwidth;
		
		if (curvalue >= maxvalue)
		{
			fillwidth = width_at_max;
		}
		else
		{
			fillwidth = (int)(((float)width_at_max / (float)maxvalue) * (float)curvalue);
			if (!fillwidth) return;
		}
		
		draw_sprite_clip_width(x, y, fill_sprite, fsframe, fillwidth);
	}
}

// draws a given number using sprite 's' as the font
// the numbers are drawn right-aligned to "x".
void DrawNumberRAlign(int x, int y, int s, int num)
{
char str[50];
int i, len;
int fontwidth = sprites[s].w;

	sprintf(str, "%d", num);
	x -= strlen(str) * fontwidth;
	
	len = strlen(str);
	for(i=0;i<len;i++)
	{
		draw_sprite(x, y, s, str[i] - '0');
		x += fontwidth;
	}
}

void DrawTwoDigitNumber(int x, int y, int num)
{
	DrawDigit(x+0, y, num/10);
	DrawDigit(x+8, y, num%10);
}

void DrawDigit(int x, int y, int digit)
{
	draw_sprite(x, y, SPR_WHITENUMBERS, digit);
}

/*
void c------------------------------() {}
*/

// Nikumaru Counter
void niku_run()
{
	if (player->equipmask & EQUIP_NIKUMARU)
	{
		if (!game.frozen && !player->inputs_locked)
		{
			if (game.counter < 300000)	// 100'00"0
				game.counter++;
		}
	}
	else
	{
		game.counter = 0;
	}
}

void niku_draw(int value, bool force_white)
{
	int clkframe = (game.counter % 30) <= 10;
	if (game.frozen || player->inputs_locked || force_white) clkframe = 0;
	
	draw_sprite(NIKU_X, NIKU_Y, SPR_NIKU_CLOCK, clkframe);
	
	int mins = (value / 3000);		// the game runs at 50 fps
	int secs = (value / 50) % 60;
	int tens = (value / 5) % 10;
	
	DrawNumber(NIKU_X, NIKU_Y, mins);
	DrawTwoDigitNumber(NIKU_X+36, NIKU_Y, secs);
	DrawDigit(NIKU_X+56, NIKU_Y, tens);
	
	draw_sprite(NIKU_X+30, NIKU_Y, SPR_NIKU_PUNC);
}

void DrawStatusBar(void)
{
int level, curxp, maxxp;
int w, x;
bool maxed_out;

	//debug("%08x", game.bossbar.object);
	//debug("%s", game.bossbar.defeated ? "true" : "false");
	
	// handle animations etc
	RunStatusBar();
	
	// draw boss bar
	if (game.bossbar.object && !game.bossbar.defeated)
	{
		#define BOSSBAR_W	198
		// BOSS_X = 32 at normal resolution
		uint32_t BOSS_X =((SCREEN_WIDTH / 2) - (BOSSBAR_W / 2) - 29);
		uint32_t BOSS_Y = (SCREEN_HEIGHT-20);
		draw_sprite(BOSS_X, BOSS_Y, SPR_TEXTBOX, 0, 0);
		draw_sprite(BOSS_X, BOSS_Y+8, SPR_TEXTBOX, 2, 0);
		draw_sprite(BOSS_X+8, BOSS_Y+4, SPR_BOSSHPICON, 0, 0);
		
		// e.g. bosses w/ multiple forms (Ballos)
		if (game.bossbar.object->hp > game.bossbar.starting_hp)
			game.bossbar.starting_hp = game.bossbar.object->hp;
		
		RunPercentBar(&game.bossbar.bar, game.bossbar.object->hp);
		DrawPercentBar(&game.bossbar.bar, BOSS_X+40, BOSS_Y+5, game.bossbar.object->hp, game.bossbar.starting_hp, BOSSBAR_W);
	}
	
	if (game.frozen || player->inputs_locked) return;
	if (fade.getstate() != FS_NO_FADE) return;
	
	if (player->hp)
	{
		if (!player->hurt_flash_state)
		{
			if (!game.debug.god)
			{
				// -- draw the health bar -----------------------------
				draw_sprite(HEALTH_X, HEALTH_Y, SPR_HEALTHBAR, 0, 0);
				
				DrawPercentBar(&PHealthBar, HEALTHFILL_X, HEALTHFILL_Y, player->hp, player->maxHealth, HEALTHFILL_MAXLEN);
				
				// draw the health in numbers
				DrawNumberRAlign(HEALTH_X+24, HEALTH_Y, SPR_WHITENUMBERS, PHealthBar.displayed_value);
			}
			
			// -- draw the XP bar ---------------------------------
			level = player->weapons[player->curWeapon].level;
			curxp = player->weapons[player->curWeapon].xp;
			maxxp = player->weapons[player->curWeapon].max_xp[level];
			
			if (player->curWeapon == WPN_NONE)
			{
				curxp = 0;
				maxxp = 1;
			}
			
			// draw XP bar and fill it
			draw_sprite(XPBAR_X+slide.lv_offset, XPBAR_Y, SPR_XPBAR, FRAME_XP_BAR, 0);
			
			maxed_out = ((curxp == maxxp) && level == 2);
			if (!maxed_out)
				DrawPercentage(XPBAR_X+slide.lv_offset, XPBAR_Y, SPR_XPBAR, FRAME_XP_FILL, curxp, maxxp, sprites[SPR_XPBAR].w);
			
			// draw the white flashing if we just got more XP
			// the time-left and flash-state are in separate variables--
			// otherwise the Spur will not flash XP bar
			if (statusbar.xpflashcount)
			{
				if (++statusbar.xpflashstate & 2)
				{
					draw_sprite(XPBAR_X+slide.lv_offset, XPBAR_Y, SPR_XPBAR, FRAME_XP_FLASH, 0);
				}
				
				statusbar.xpflashcount--;
			}
			else statusbar.xpflashstate = 0;
			
			// draw "MAX"
			if (maxed_out)
				draw_sprite(XPBAR_X+slide.lv_offset, XPBAR_Y, SPR_XPBAR, FRAME_XP_MAX, 0);
			
			// Level Number
			DrawWeaponLevel(HEALTH_X + slide.lv_offset, XPBAR_Y, player->curWeapon);
		}
		
		// -- draw the weapon bar -----------------------------
		// draw current weapon
		if (player->curWeapon != WPN_NONE)
			draw_sprite(CURWEAPON_X + slide.wpn_offset, WEAPONBAR_Y, SPR_ARMSICONS, slide.firstWeapon, 0);
		
		// draw ammo, note we draw ammo of firstweapon NOT current weapon, for slide effect
		DrawWeaponAmmo((AMMO_X + slide.wpn_offset + slide.ammo_offset), AMMO_Y, slide.firstWeapon);
		
		// draw other weapons
		w = slide.firstWeapon;
		x = STATUS_X + 64 + slide.wpn_offset + 1;
		for(;;)
		{
			if (++w >= WPN_COUNT) w = 0;
			if (w==slide.firstWeapon) break;
			
			if (player->weapons[w].hasWeapon)
			{
				draw_sprite(x, WEAPONBAR_Y, SPR_ARMSICONS, w, RIGHT);
				x += 16;
			}
		}
		
		DrawAirLeft((SCREEN_WIDTH/2) - (5*8), ((SCREEN_HEIGHT)/2)-16);
	}
}
