
#include "nx.h"

#include <cstdarg>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <direct.h>
#include <io.h>
#endif
//#include "main.h"
#include "game.h"
#include "graphics/Renderer.h"
#include "input.h"
#include "map.h"
#include "profile.h"
#include "settings.h"
#include "statusbar.h"
#include "trig.h"
#include "tsc.h"

#include <SDL_mixer.h>
using namespace NXE::Graphics;
#include "ResourceManager.h"
#include "caret.h"
#include "common/misc.h"
#include "console.h"
#include "screeneffect.h"
#include "sound/SoundManager.h"
#include "Utils/Logger.h"
using namespace NXE::Utils;

#if defined(__SWITCH__)
#include <switch.h>
#include <iostream>
#endif

#if defined(__VITA__)
#include <psp2/kernel/threadmgr.h>
extern "C"
{
    unsigned int sleep(unsigned int seconds)
    {
        sceKernelDelayThread(seconds*1000*1000);
        return 0;
    }

    int usleep(useconds_t usec)
    {
        sceKernelDelayThread(usec);
        return 0;
    }
}
#endif

using namespace NXE::Sound;

int fps                  = 0;
static int fps_so_far    = 0;
static uint32_t fpstimer = 0;

#define GAME_WAIT (1000 / GAME_FPS) // sets framerate
int framecount    = 0;
bool freezeframe  = false;
int flipacceltime = 0;

static void fatal(const char *str)
{
  LOG_CRITICAL("fatal: '%s'", str);

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
    fpstimer   = SDL_GetTicks();
    fps        = (fps_so_far << 1);
    fps_so_far = 0;
  }

  char fpstext[64];
  sprintf(fpstext, "%d fps", fps);

  int x = (Renderer::getInstance()->screenWidth - 4) - Renderer::getInstance()->font.getWidth(fpstext);
  Renderer::getInstance()->font.draw(x, 4, fpstext, 0x00FF00, true);
}

static inline void run_tick()
{
  static bool can_tick       = true;
#if defined(DEBUG)
  static bool last_freezekey = false;
  static bool last_framekey  = false;
#endif
  static int frameskip       = 0;

  input_poll();

  // input handling for a few global things

  if (justpushed(F9KEY))
  {
    Renderer::getInstance()->saveScreenshot();
  }

  // freeze frame
#if defined(DEBUG)
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
      framecount  = 0;
    }
  }

  last_freezekey = inputs[FREEZE_FRAME_KEY];
  last_framekey  = inputs[FRAME_ADVANCE_KEY];

  // fast-forward key (F5)

  if (inputs[FFWDKEY])
  {
    game.ffwdtime = 2;
  }

#endif

  if (can_tick)
  {
    game.tick();

    if (freezeframe)
    {
      char buf[1024];
      sprintf(buf, "[] Tick %d", framecount++);
      Renderer::getInstance()->font.draw(4, (Renderer::getInstance()->screenHeight - Renderer::getInstance()->font.getHeight() - 4), buf, 0x00FF00, true);
      sprintf(buf, "Left: %d, Right: %d, JMP: %d, FR: %d, ST: %d", inputs[LEFTKEY], inputs[RIGHTKEY], inputs[JUMPKEY],
              inputs[FIREKEY], inputs[STRAFEKEY]);
      Renderer::getInstance()->font.draw(80, (Renderer::getInstance()->screenHeight - Renderer::getInstance()->font.getHeight() - 4), buf, 0x00FF00, true);
      can_tick = false;
    }

    if (settings->show_fps)
    {
      update_fps();
    }

    if (!flipacceltime)
    {
      Renderer::getInstance()->flip();
    }
    else
    {
      flipacceltime--;
      if (--frameskip < 0)
      {
        Renderer::getInstance()->flip();
        frameskip = 256;
      }
    }

    memcpy(lastinputs, inputs, sizeof(lastinputs));
  }
  else
  { // frame is frozen; don't hog CPU
    SDL_Delay(20);
  }

  SoundManager::getInstance()->runFade();
}

void AppMinimized(void)
{
  LOG_DEBUG("Game minimized or lost focus--pausing...");
  NXE::Sound::SoundManager::getInstance()->pause();
  for (;;)
  {
    if (Renderer::getInstance()->isWindowVisible())
    {
      break;
    }

    input_poll();
    SDL_Delay(20);
  }
  NXE::Sound::SoundManager::getInstance()->resume();
  LOG_DEBUG("Focus regained, resuming play...");
}

void gameloop(void)
{
  int32_t nexttick = 0;

  game.switchstage.mapno = -1;

  while (game.running && game.switchstage.mapno < 0)
  {
    // get time until next tick
    int32_t curtime       = SDL_GetTicks();
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
      if (!Renderer::getInstance()->isWindowVisible())
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
  LOG_DEBUG("= Beginning new game =");

  memset(game.flags, 0, sizeof(game.flags));
  memset(game.skipflags, 0, sizeof(game.skipflags));
  textbox.StageSelect.ClearSlots();

  game.quaketime = game.megaquaketime = 0;
  game.showmapnametime                = 0;
  game.debug.god                      = 0;
  game.running                        = true;
  game.frozen                         = false;

  // fully re-init the player object
  Objects::DestroyAll(true);
  game.createplayer();

  player->maxHealth = 3;
  player->hp        = player->maxHealth;

  game.switchstage.mapno        = STAGE_START_POINT;
  game.switchstage.playerx      = 10;
  game.switchstage.playery      = 8;
  game.switchstage.eventonentry = (with_intro) ? 200 : 91;

  fade.set_full(FADE_OUT);
}

int main(int argc, char *argv[])
{
  bool error            = false;
  bool freshstart;

#if defined(_WIN32)
  char *basepath = SDL_GetBasePath();
  _chdir(basepath);
  SDL_free(basepath);
#elif not defined(__VITA__) && not defined(__SWITCH__)
  char *basepath = SDL_GetBasePath();
  chdir(basepath);
  SDL_free(basepath);
#endif

#if defined(__SWITCH__)
  if (romfsInit() != 0)
  {
    std::cerr << "romfsInit() failed" << std::endl;
    return 1;
  }
#endif

  (void)ResourceManager::getInstance();

  Logger::init(ResourceManager::getInstance()->getPrefPath("debug.log"));

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
  {
    LOG_CRITICAL("ack, sdl_init failed: {}.", SDL_GetError());
    return 1;
  }

  // start up inputs first thing because settings_load may remap them
  input_init();

  // load settings, or at least get the defaults,
  // so we know the initial screen resolution.
  settings_load();

  if (!Renderer::getInstance()->init(settings->resolution))
  {
    fatal("Failed to initialize graphics.");
    return 1;
  }
  Renderer::getInstance()->setFullscreen(settings->fullscreen);

  //	if (check_data_exists())
  //	{
  //		return 1;
  //	}

  Renderer::getInstance()->showLoadingScreen();

  if (!SoundManager::getInstance()->init())
  {
    fatal("Failed to initialize sound.");
    return 1;
  }

  if (trig_init())
  {
    fatal("Failed trig module init.");
    return 1;
  }

  if (textbox.Init())
  {
    fatal("Failed to initialize textboxes.");
    return 1;
  }
  if (Carets::init())
  {
    fatal("Failed to initialize carets.");
    return 1;
  }

  if (game.init())
    return 1;
  if (!game.tsc->Init())
  {
    fatal("Failed to initialize script engine.");
    return 1;
  }
  game.setmode(GM_NORMAL);
  // set null stage just to have something to do while we go to intro
  game.switchstage.mapno = 0;

  char *profile_name = GetProfileName(settings->last_save_slot);
  if (settings->skip_intro && file_exists(profile_name))
    game.switchstage.mapno = LOAD_GAME;
  else
    game.setmode(GM_INTRO);

  SDL_free(profile_name);

  // for debug
  if (game.paused)
  {
    game.switchstage.mapno        = 0;
    game.switchstage.eventonentry = 0;
  }

  game.running = true;
  freshstart   = true;

  LOG_INFO("Entering main loop...");

  while (game.running)
  {
    // SSS/SPS persists across stage transitions until explicitly
    // stopped, or you die & reload. It seems a bit risky to me,
    // but that's the spec.
    if (game.switchstage.mapno >= MAPNO_SPECIALS)
    {
      NXE::Sound::SoundManager::getInstance()->stopLoopSfx();
      //			StopLoopSounds();
    }

    // enter next stage, whatever it may be
    if (game.switchstage.mapno == LOAD_GAME || game.switchstage.mapno == LOAD_GAME_FROM_MENU)
    {
      if (game.switchstage.mapno == LOAD_GAME_FROM_MENU)
        freshstart = true;

      LOG_DEBUG("= Loading game =");
      if (game_load(settings->last_save_slot))
      {
        fatal("savefile error");
        goto ingame_error;
      }
      fade.set_full(FADE_IN);
    }
    else if (game.switchstage.mapno == TITLE_SCREEN)
    {
      LOG_DEBUG("= Title screen =");
      game.curmap = TITLE_SCREEN;
    }
    else
    {
      if (game.switchstage.mapno == NEW_GAME || game.switchstage.mapno == NEW_GAME_FROM_MENU)
      {
        bool show_intro = (game.switchstage.mapno == NEW_GAME_FROM_MENU);
        InitNewGame(show_intro);
      }

      // slide weapon bar on first intro to Start Point
      if (game.switchstage.mapno == STAGE_START_POINT && game.switchstage.eventonentry == 91)
      {
        freshstart = true;
      }

      // switch maps
      if (load_stage(game.switchstage.mapno))
        goto ingame_error;

      player->x = (game.switchstage.playerx * TILE_W) * CSFI;
      player->y = (game.switchstage.playery * TILE_H) * CSFI;
    }

    // start the level
    if (game.initlevel())
      return 1;

    if (freshstart)
      weapon_introslide();

    gameloop();
    game.stageboss.OnMapExit();
    freshstart = false;
  }

shutdown:;
  game.tsc->Close();
  game.close();
  Carets::close();

  input_close();
  textbox.Deinit();
  NXE::Sound::SoundManager::getInstance()->shutdown();
  Renderer::getInstance()->close();
#if defined(__SWITCH__)
  romfsExit();
#endif
  SDL_Quit();
  return error;

ingame_error:;
  LOG_CRITICAL("");
  LOG_CRITICAL(" ************************************************");
  LOG_CRITICAL(" * An in-game error occurred. Game shutting down.");
  LOG_CRITICAL(" ************************************************");
  error = true;
  goto shutdown;
}
