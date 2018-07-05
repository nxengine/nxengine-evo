
#ifndef _GAME_H
#define _GAME_H

#include "statusbar.h"
#include "stageboss.h"
#include "TextBox/TextBox.h"
#include "ObjManager.h"
#include "tsc.h"
#include "i18n/translate.h"

#define GAME_FPS		50

// for UpdateBlockedStates
#define RIGHTMASK		0x01
#define LEFTMASK		0x02
#define UPMASK			0x04
#define DOWNMASK		0x08
#define ALLDIRMASK		(LEFTMASK | RIGHTMASK | UPMASK | DOWNMASK)

// highest addressable flag by tsc scripts etc
#define NUM_GAMEFLAGS		8000

#define MAPNO_SPECIALS		1000
#define LOAD_GAME			1000	// switchstage.mapno is set to this to load a game
#define NEW_GAME			1001
#define NEW_GAME_FROM_MENU	1002	// new game (include Kazuma cutscene)
#define LOAD_GAME_FROM_MENU	1003	// load game (from title screen, include weapon slide)
#define START_REPLAY		1004
#define TITLE_SCREEN		1005	// dynamic title screen background(s)

// game modes (changes *tickfunction)
enum GameModes
{
	GM_NONE,			// default mode at startup & shutdown
	GM_NORMAL,			// playing the game
	GM_INVENTORY,		// in inventory screen
	GM_MAP_SYSTEM,		// viewing Map System
	GM_ISLAND,			// XX1 good-ending island-crash cutscene
	GM_CREDITS,			// <CRE credits
	GM_INTRO,			// intro
	GM_TITLE,			// title screen
	
	GP_PAUSED,			// pausemode: Pause (use game.pause())
	GP_OPTIONS,			// pausemode: Options (use game.pause())
	
	NUM_GAMEMODES
};

// note: this structure is memsetted at 0 at startup.
// ensure it doesn't contain any non-POD types that would be harmed by this.
struct Game
{
	bool running;
	bool frozen;
	
	int mode;
	int paused;
	
	int curmap;
	int showmapnametime;
	int mapname_x;
	
	uint32_t counter;	// Nikumaru counter value
	
	TSC* tsc;
	I18N* lang;
	
	struct
	{
		bool god;
		//bool debugmode;
		bool infinite_damage;
		bool DrawBoundingBoxes;
	} debug;
	
	// if mapno becomes >= 0 the stage ends and we switch to the new stage
	struct
	{
		int mapno;
		int playerx, playery;
		int eventonentry;
		int param;
	} switchstage;
	
	// game flags and skipflags as set by scripts
	bool flags[NUM_GAMEFLAGS];
	bool skipflags[NUM_GAMEFLAGS];
	
	// earthquake effect; <QUA command
	int quaketime;
	int megaquaketime;
	
	// stuff for boss bar
 	struct
	{
		Object *object;			// if != NULL, a boss bar is shown displaying this object's remaining health
		int starting_hp;		// HP boss had when <BSL was called
		bool defeated;			// set to true when boss is defeated
		PercentBar bar;
	} bossbar;
	
	StageBossManager stageboss;
	
	// set by enemies in Labyrinth M and by Almond Core to tell
	// curly where to shoot at (it's like a hint)
	struct
	{
		int x, y, timeleft;
	} curlytarget;
	
	int fullscreen;
	int ffwdtime;		// debug option: disables speed-limiting for ffwdtime ticks
	
// ---------------------------------------

	// static member functions--not private (Game is an object, not a namespace)
	bool init();
	bool initlevel();
	bool createplayer();
	
	void switchmap(int mapno, int scriptno=0, int px=0, int py=0);
	void reset();
	
	bool setmode(int newmode, int param=0, bool force=false);
	bool pause(int pausemode, int param=0);
	void tick();
	
	void close();
};

// NPC flags definitions
#define FLAG_SOLID_MUSHY			0x0001	// object blocks player but is a little "mushy" (normal solid state for enemies)
#define FLAG_IGNORETILE44			0x0002
#define FLAG_INVULNERABLE			0x0004
#define FLAG_IGNORE_SOLID			0x0008
#define FLAG_BOUNCY					0x0010	// when SOLID_BRICK also set, acts like a mini trampoline
#define FLAG_SHOOTABLE				0x0020
#define FLAG_SOLID_BRICK			0x0040	// object's entire bbox is rock-solid, just like a solid tile
#define FLAG_NOREARTOPATTACK		0x0080
#define FLAG_SCRIPTONTOUCH			0x0100
#define FLAG_SCRIPTONDEATH			0x0200
#define FLAG_DROP_POWERUPS_DONTUSE	0x0400	// not used here because it doesn't seem to be set on some npc.tbl entries which DO in fact spawn powerups; see the nxflag which replaces it
#define FLAG_APPEAR_ON_FLAGID		0x0800
#define FLAG_FACES_RIGHT			0x1000
#define FLAG_SCRIPTONACTIVATE		0x2000
#define FLAG_DISAPPEAR_ON_FLAGID	0x4000
#define FLAG_SHOW_FLOATTEXT			0x8000

// NXEngine flag definitions
#define NXFLAG_FOLLOW_SLOPE			0x0001	// enable moving up/down slopes when moving horizontally
#define NXFLAG_SLOW_X_WHEN_HURT		0x0002	// move at half X speed when shaking from damage
#define NXFLAG_SLOW_Y_WHEN_HURT		0x0004	// move at half Y speed when shaking from damage
#define NXFLAG_THUD_ON_RIDING		0x0008	// if set there is a "thud" sound when player lands on it
#define NXFLAG_NO_RESET_YINERTIA	0x0010	// don't zero yinertia on blocku/blockd
#define NXFLAG_CONSOLE_ANIMATE		0x0020	// spawned at console and is implicit target of subsequent animate commands

#define NXFLAG_SLOW_WHEN_HURT		(NXFLAG_SLOW_X_WHEN_HURT | NXFLAG_SLOW_Y_WHEN_HURT)

extern Game game;
#define _(x) game.lang->translate(x)
extern TextBox textbox;

extern Object *onscreen_objects[MAX_OBJECTS];
extern int nOnscreenObjects;

void debug(const char *fmt, ...);
void quake(int quaketime, int snd=-1);
void megaquake(int quaketime, int snd=-1);

struct Profile;
bool game_load(int num);
bool game_load(Profile *p);
bool game_save(int num);
bool game_save(Profile *p);
void DrawScene(void);

void game_tick_normal(void);
void AssignExtraSprites(void);


#endif
