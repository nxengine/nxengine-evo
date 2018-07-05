
#include "nx.h"
#include <cstdarg>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif
//#include "main.h"
#include "game.h"
#include "map.h"
#include "profile.h"
#include "settings.h"
#include "statusbar.h"
#include "tsc.h"
#include "input.h"
#include "trig.h"
#include "graphics/graphics.h"
using namespace Graphics;
#include "graphics/font.h"
#include "graphics/screenshot.h"
#include "sound/sound.h"
#include "sound/org.h"
#include "common/stat.h"
#include "common/misc.h"
#include "caret.h"
#include "console.h"
#include "screeneffect.h"
#include "ResourceManager.h"


int fps = 0;
static int fps_so_far = 0;
static uint32_t fpstimer = 0;

#define GAME_WAIT			(1000/GAME_FPS)	// sets framerate
int framecount = 0;
bool freezeframe = false;
int flipacceltime = 0;

static void fatal(const char *str)
{
	staterr("fatal: '%s'", str);
	
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", str, NULL);
}

/*static bool check_data_exists()
{
char fname[MAXPATHLEN];

	sprintf(fname, "%s/npc.tbl", data_dir);
	if (file_exists(fname)) return 0;
	
	fatal("Missing \"data\" directory.\nPlease copy it over from a Doukutsu installation.");
	
	return 1;
}*/

void update_fps()
{
	fps_so_far++;
	
	if ((SDL_GetTicks() - fpstimer) >= 500)
	{
		fpstimer = SDL_GetTicks();
		fps = (fps_so_far << 1);
		fps_so_far = 0;
	}
	
	char fpstext[64];
	sprintf(fpstext, "%d fps", fps);
	
	int x = (SCREEN_WIDTH - 4) - GetFontWidth(fpstext, true);
	font_draw(x, 4, fpstext, 0x00FF00, true);
}

static inline void run_tick()
{
static bool can_tick = true;
static bool last_freezekey = false;
static bool last_framekey = false;
static int frameskip = 0;

	input_poll();
	
	// input handling for a few global things
	
	if (justpushed(F9KEY))
	{
	    SaveScreenshot();
	}
	
	// freeze frame
	if (settings->enable_debug_keys)
	{
		if (inputs[FREEZE_FRAME_KEY] && !last_freezekey)
		{
			can_tick = true;
			freezeframe ^= 1;
			framecount = 0;
		}
		
		if (inputs[FRAME_ADVANCE_KEY] && !last_framekey)
		{
			can_tick = true;
			if (!freezeframe)
			{
				freezeframe = 1;
				framecount = 0;
			}
		}
		
		last_freezekey = inputs[FREEZE_FRAME_KEY];
		last_framekey = inputs[FRAME_ADVANCE_KEY];
	}
	
	// fast-forward key (F5)
	if (inputs[FFWDKEY] && settings->enable_debug_keys)
	{
		game.ffwdtime = 2;
	}
	
	if (can_tick)
	{
		game.tick();
		
		if (freezeframe)
		{
			char buf[1024];
			sprintf(buf, "[] Tick %d", framecount++);
			font_draw(4, (SCREEN_HEIGHT-GetFontHeight()-4), buf, 0x00FF00, true);
			sprintf(buf, "Left: %d, Right: %d, JMP: %d, FR: %d", inputs[LEFTKEY], inputs[RIGHTKEY], inputs[JUMPKEY], inputs[FIREKEY]);
			font_draw(80, (SCREEN_HEIGHT-GetFontHeight()-4), buf, 0x00FF00, true);
			can_tick = false;
		}
		
		if (settings->show_fps)
		{
			update_fps();
		}
		
		if (!flipacceltime)
		{
			screen->Flip();
		}
		else
		{
			flipacceltime--;
			if (--frameskip < 0)
			{
				screen->Flip();
				frameskip = 256;
			}
		}
		
		memcpy(lastinputs, inputs, sizeof(lastinputs));
	}
	else
	{	// frame is frozen; don't hog CPU
		SDL_Delay(20);
	}
	
	music_run_fade();
}

void AppMinimized(void)
{
	stat("Game minimized or lost focus--pausing...");
	SDL_PauseAudio(1);
	
	for(;;)
	{
        if (Graphics::WindowVisible())
		{
			break;
		}
		
		input_poll();
		SDL_Delay(20);
	}
	
	SDL_PauseAudio(0);
	stat("Focus regained, resuming play...");
}

void gameloop(void)
{
int32_t nexttick = 0;

	game.switchstage.mapno = -1;
	
	while(game.running && game.switchstage.mapno < 0)
	{
		// get time until next tick
		int32_t curtime = SDL_GetTicks();
		int32_t timeRemaining = nexttick - curtime;
		
		if (timeRemaining <= 0 || game.ffwdtime)
		{
			run_tick();
			
			// try to "catch up" if something else on the system bogs us down for a moment.
			// but if we get really far behind, it's ok to start dropping frames
			if (game.ffwdtime)
				game.ffwdtime--;
			
			nexttick = curtime + GAME_WAIT;
			
			// pause game if window minimized
            if(!Graphics::WindowVisible())
			{
				AppMinimized();
				nexttick = 0;
			}
		}
		else
		{
			// don't needlessly hog CPU, but don't sleep for entire
			// time left, some CPU's/kernels will fall asleep for
			// too long and cause us to run slower than we should
			timeRemaining /= 2;
			if (timeRemaining)
				SDL_Delay(timeRemaining);
		}
	}
}

void InitNewGame(bool with_intro)
{
	stat("= Beginning new game =");
	
	memset(game.flags, 0, sizeof(game.flags));
	memset(game.skipflags, 0, sizeof(game.skipflags));
	textbox.StageSelect.ClearSlots();
	
	game.quaketime = game.megaquaketime = 0;
	game.showmapnametime = 0;
	game.debug.god = 0;
	game.running = true;
	game.frozen = false;
	
	// fully re-init the player object
	Objects::DestroyAll(true);
	game.createplayer();
	
	player->maxHealth = 3;
	player->hp = player->maxHealth;
	
	game.switchstage.mapno = STAGE_START_POINT;
	game.switchstage.playerx = 10;
	game.switchstage.playery = 8;
	game.switchstage.eventonentry = (with_intro) ? 200 : 91;
	
	fade.set_full(FADE_OUT);
}

int main(int argc, char *argv[])
{
bool inhibit_loadfade = false;
bool error = false;
bool freshstart;
	
	char* basepath = SDL_GetBasePath();
	
#if defined(_WIN32)
	_chdir(basepath);
#else
	chdir(basepath);
#endif
	SDL_free(basepath);
	
	char* prefpath = SDL_GetPrefPath("nxengine", "nxengine-evo");
	std::string logpath = std::string(prefpath) + "debug.log";
	SDL_free(prefpath);
	
	SetLogFilename(logpath.c_str());
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		staterr("ack, sdl_init failed: %s.", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);
	
	// start up inputs first thing because settings_load may remap them
	input_init();
	
	// load settings, or at least get the defaults,
	// so we know the initial screen resolution.
	settings_load();
	
	(void)ResourceManager::getInstance();
	
	if (Graphics::init(settings->resolution)) { fatal("Failed to initialize graphics."); return 1; }
	Graphics::SetFullscreen(settings->fullscreen);
	if (font_init()) { fatal("Failed to load font."); return 1; }
	
	
//	if (check_data_exists())
//	{
//		return 1;
//	}
	
	Graphics::ShowLoadingScreen();
	if (sound_init()) { fatal("Failed to initialize sound."); return 1; }
	if (trig_init()) { fatal("Failed trig module init."); return 1; }
	
	if (textbox.Init()) { fatal("Failed to initialize textboxes."); return 1; }
	if (Carets::init()) { fatal("Failed to initialize carets."); return 1; }
	
	if (game.init()) return 1;
	if (!game.tsc->Init()) { fatal("Failed to initialize script engine."); return 1; }
	game.setmode(GM_NORMAL);
	// set null stage just to have something to do while we go to intro
	game.switchstage.mapno = 0;
	
	char* profile_name = GetProfileName(settings->last_save_slot);
	if (settings->skip_intro && file_exists(profile_name))
		game.switchstage.mapno = LOAD_GAME;
	else
		game.setmode(GM_INTRO);
	
	SDL_free(profile_name);
	
	// for debug
	if (game.paused) { game.switchstage.mapno = 0; game.switchstage.eventonentry = 0; }
	if (game.switchstage.mapno == LOAD_GAME) inhibit_loadfade = true;
	
	game.running = true;
	freshstart = true;
	
	stat("Entering main loop...");
	
	
	while(game.running)
	{
		// SSS/SPS persists across stage transitions until explicitly
		// stopped, or you die & reload. It seems a bit risky to me,
		// but that's the spec.
		if (game.switchstage.mapno >= MAPNO_SPECIALS)
		{
			StopLoopSounds();
		}
		
		// enter next stage, whatever it may be
		if (game.switchstage.mapno == LOAD_GAME || \
			game.switchstage.mapno == LOAD_GAME_FROM_MENU)
		{
			if (game.switchstage.mapno == LOAD_GAME_FROM_MENU)
				freshstart = true;
			
			stat("= Loading game =");
			if (game_load(settings->last_save_slot))
			{
				fatal("savefile error");
				goto ingame_error;
			}
			
			if (!inhibit_loadfade) fade.Start(FADE_IN, FADE_CENTER);
			else inhibit_loadfade = false;
		}
		else if (game.switchstage.mapno == TITLE_SCREEN)
		{
			stat("= Title screen =");
			game.curmap = TITLE_SCREEN;
		}
		else
		{
			if (game.switchstage.mapno == NEW_GAME || \
				game.switchstage.mapno == NEW_GAME_FROM_MENU)
			{
				bool show_intro = (game.switchstage.mapno == NEW_GAME_FROM_MENU);
				InitNewGame(show_intro);
			}
			
			// slide weapon bar on first intro to Start Point
			if (game.switchstage.mapno == STAGE_START_POINT && \
				game.switchstage.eventonentry == 91)
			{
				freshstart = true;
			}
			
			// switch maps
			if (load_stage(game.switchstage.mapno)) goto ingame_error;

			player->x = (game.switchstage.playerx * TILE_W) * CSFI;
			player->y = (game.switchstage.playery * TILE_H) * CSFI;
		}
		
		// start the level
		if (game.initlevel()) return 1;
		
		if (freshstart)
			weapon_introslide();
		
		gameloop();
		game.stageboss.OnMapExit();
		freshstart = false;
	}
	
shutdown: ;
	game.tsc->Close();
	game.close();
	Carets::close();
	
	Graphics::close();
	input_close();
	font_close();
	sound_close();
	textbox.Deinit();
	return error;
	
ingame_error: ;
	stat("");
	stat(" ************************************************");
	stat(" * An in-game error occurred. Game shutting down.");
	stat(" ************************************************");
	error = true;
	goto shutdown;
}



