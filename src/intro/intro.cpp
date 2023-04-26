
#include "intro.h"

#include "../ai/ai.h"
#include "../ai/stdai.h"
#include "../caret.h"
#include "../common/misc.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../map.h"
#include "../nx.h"
#include "../player.h"
#include "../screeneffect.h"
#include "../sound/SoundManager.h"
#include "../ResourceManager.h"
#include "../tsc.h"
using namespace NXE::Graphics;

static int blanktimer;
#define EXIT_DELAY 20 // delay between intro and title screen

bool intro_init(int param)
{
  NXE::Sound::SoundManager::getInstance()->music(0);
  fade.set_full(FADE_OUT);

  game.switchstage.mapno        = STAGE_KINGS;
  game.switchstage.playerx      = 0;
  game.switchstage.playery      = 0;
  game.switchstage.eventonentry = 100;
  player->hide                  = true;

  return 0;
}

void intro_tick()
{
  if (blanktimer == 0 && ResourceManager::getInstance()->isMod() && ResourceManager::getInstance()->mod().skip_intro)
  {
    game.tsc->StopScripts();
    blanktimer = EXIT_DELAY;
  }
  if (blanktimer > 0)
  {
    Renderer::getInstance()->clearScreen(BLACK);

    if (--blanktimer == 0)
      game.setmode(GM_TITLE);
    return;
  }

  game_tick_normal();
  player->y = 0;

  // when script ends with WAI9999, detect it and move on to title screen
  ScriptInstance *script = game.tsc->GetCurrentScriptInstance();
  if ((script && script->delaytimer == 9999) || buttonjustpushed())
  {
    game.tsc->StopScripts();
    blanktimer = EXIT_DELAY;
  }

  // lower time for startup
  if (script && script->delaytimer && script->ip == 5)
  {
    game.showmapnametime = 0;
    if (script->delaytimer > 20)
      script->delaytimer = 20;
  }
}

/*
void c------------------------------() {}
*/

/*
void c------------------------------() {}
*/

// misery/balrog in bubble
void ai_intro_kings(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state = 1;

      if (o->dir == LEFT)
      {
        o->timer = 25;
        o->y -= 0x640;
      }
    }
    case 1:
    {
      if (++o->timer >= 50)
      {
        o->timer = 0;
        o->timer2 ^= 1;
      }

      if (o->timer2)
      {
        o->y += 0x40;
      }
      else
      {
        o->y -= 0x40;
      }
    }
    break;
  }
}

// demon crown on throne
void ai_intro_crown(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->x += (8 * CSFI);
      o->y += (14 * CSFI);
      o->state = 1;
    }
    case 1:
    {
      if ((++o->timer % 8) == 1)
      {
        effect(o->x + random(-8, 8) * CSFI, o->y + (8 * CSFI), EFFECT_GHOST_SPARKLE);
      }
    }
    break;
  }
}

void ai_intro_doctor(Object *o)
{

  switch (o->state)
  {
    case 0:
    {
      o->y -= (8 * CSFI);
      o->state = 1;
    }
    case 1:
    {
      o->frame = 0;
    }
    break;

    case 10: // chuckle; facing screen
    {
      o->state     = 11;
      o->frame     = 0;
      o->animtimer = 0;
      o->timer2    = 0;
    }
    case 11:
    {
      ANIMATE_FWD(6);
      if (o->frame > 1)
      {
        o->frame = 0;
        if (++o->timer2 > 7)
          o->state = 1;
      }
    }
    break;

    case 20: // walk
    {
      o->state     = 21;
      o->frame     = 2;
      o->animtimer = 0;
    }
    case 21:
    {
      ANIMATE(10, 2, 5);
      o->x += 0x100;
    }
    break;

    case 30: // face away
    {
      o->frame = 6;
      o->state = 31;
    }
    break;

    case 40: // chuckle; facing away
    {
      o->state     = 41;
      o->frame     = 6;
      o->animtimer = 0;
      o->timer2    = 0;
    }
    case 41:
    {
      ANIMATE_FWD(6);
      if (o->frame > 7)
      {
        o->frame = 6;
        if (++o->timer2 > 7)
          o->state = 30;
      }
    }
    break;
  }
}

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_INTRO_KINGS, ai_intro_kings);
  ONTICK(OBJ_INTRO_CROWN, ai_intro_crown);
  ONTICK(OBJ_INTRO_DOCTOR, ai_intro_doctor);
}
