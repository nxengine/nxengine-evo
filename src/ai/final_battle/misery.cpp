#include "misery.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Tileset.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../hell/ballos_priest.h"
#include "../stdai.h"
#include "../sym/smoke.h"
#include "doctor_frenzied.h"

#define STATE_FIGHTING 100 // script-triggered, must be constant

#define STATE_FLASH_FOR_SPELL 200 // flashes then casts either FIRE_SHOTS or SUMMON_BLOCk
#define STATE_FIRE_SHOTS 210      // fires black shots at player
#define STATE_SUMMON_BLOCK 220    // summons falling block over player's head
#define STATE_SUMMON_BALLS 230    // summons black-lightning balls
#define STATE_TP_AWAY 240         // teleports away then reappears

#define STATE_DEFEATED 1000 // script-triggered, must be constant

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_BOSS_MISERY, ai_boss_misery);
  ONTICK(OBJ_MISERY_PHASE, ai_misery_phase);
  ONTICK(OBJ_MISERY_SHOT, ai_generic_angled_shot);

  ONTICK(OBJ_MISERY_RING, ai_misery_ring);
  AFTERMOVE(OBJ_MISERY_RING, aftermove_misery_ring);

  ONTICK(OBJ_MISERY_BALL, ai_misery_ball);
  ONTICK(OBJ_BLACK_LIGHTNING, ai_black_lightning);
}

/*
void c------------------------------() {}
*/

void ai_boss_misery(Object *o)
{
  /*debug("state: %d", o->state);
  debug("timer: %d", o->timer);
  debug("timer2: %d", o->timer2);*/

  switch (o->state)
  {
    // fight begin and default/base state
    case STATE_FIGHTING:
    {
      o->flags |= FLAG_SHOOTABLE;
      o->savedhp = o->hp;

      o->timer    = 0;
      o->frame    = 0;
      o->xinertia = 0;
      o->state++;
    }
    case STATE_FIGHTING + 1:
    {
      FACEPLAYER;

      o->yinertia += (o->y < o->ymark) ? 0x20 : -0x20;
      LIMITY(0x200);

      if (++o->timer > 200 || (o->savedhp - o->hp) >= 80)
      {
        o->state = STATE_FLASH_FOR_SPELL;
        o->timer = 0;
      }
    }
    break;
  }

  run_spells(o);
  run_teleport(o);

  run_intro(o);
  run_defeated(o);

  LIMITX(0x200);
  LIMITY(0x400);
}

// her 3 attacks: black shots, black balls, and summon falling block.
static void run_spells(Object *o)
{
  switch (o->state)
  {
    // flashes for spell...
    // then either fires shots or casts the falling-block spell
    case STATE_FLASH_FOR_SPELL:
    {
      o->flags &= ~FLAG_SHOOTABLE;
      o->xinertia = 0;
      o->yinertia = 0;

      o->timer = 0;
      o->state++;
    }
    case STATE_FLASH_FOR_SPELL + 1:
    {
      o->timer++;
      o->frame = 5 + (o->timer & 1);

      if (o->timer > 30)
      {
        o->timer = 0;
        o->frame = 4;

        if (++o->timer2 >= 3)
        {
          o->state  = STATE_SUMMON_BLOCK;
          o->timer2 = 0;
        }
        else
        {
          o->state = STATE_FIRE_SHOTS;
        }
      }
    }
    break;

    // fire black shots at player
    case STATE_FIRE_SHOTS:
    {
      if ((++o->timer % 6) == 0)
      {
        EmFireAngledShot(o, OBJ_MISERY_SHOT, 4, 0x800);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);
      }

      if (o->timer > 30)
      {
        o->timer = 0;
        o->state = STATE_TP_AWAY;
      }
    }
    break;

    // summon falling block
    case STATE_SUMMON_BLOCK:
    {
      if (++o->timer == 10)
      {
        int x         = player->x - (8 * CSFI);
        int y         = player->y - (64 * CSFI);
        Object *block = CreateObject(x, y, OBJ_FALLING_BLOCK);
        block->sprite = SPR_BALCONY_BLOCK_LARGE;
        block->dir    = DOWN; // tell block it was spawned by Misery
      }

      if (o->timer > 30)
      {
        o->state = STATE_TP_AWAY;
        o->timer = 0;
      }
    }
    break;

    // summon black balls
    case STATE_SUMMON_BALLS:
    {
      FACEPLAYER;
      o->frame = 4;

      o->timer = 0;
      o->state++;
    }
    case STATE_SUMMON_BALLS + 1:
    {
      o->yinertia += (o->y < o->ymark) ? 0x20 : -0x20;
      LIMITY(0x200);

      if ((++o->timer % 24) == 0)
      {
        CreateObject(o->x, o->y + (4 * CSFI), OBJ_MISERY_BALL);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);
      }

      if (o->timer > 72)
      {
        o->state = 100;
        o->timer = 0;
      }
    }
    break;
  }
}

// runs her teleport-away and reappear states.
static void run_teleport(Object *o)
{
  switch (o->state)
  {
    // teleport away, then reappear someplace else
    case STATE_TP_AWAY:
    {
      o->state++;
      o->timer     = 0;
      o->invisible = true;
      o->flags &= ~FLAG_SHOOTABLE;

      CreateObject(o->x, o->y, OBJ_MISERY_PHASE)->dir = LEFT;
      CreateObject(o->x, o->y, OBJ_MISERY_PHASE)->dir = RIGHT;

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
    }
    case STATE_TP_AWAY + 1:
    {
      o->timer++;

      // it takes exactly 8 frames for the phase-in animation to complete
      if (o->timer == 42)
      {
        // we don't actually move until the last possible second
        // in order not to bring the floattext/damage numbers with us,
        // which gives away our position.
        o->xmark = (random(9, 31) * TILE_W) * CSFI;
        o->ymark = (random(5, 7) * TILE_H) * CSFI;

        CreateObject(o->xmark + 0x2000, o->ymark, OBJ_MISERY_PHASE)->dir = LEFT;
        CreateObject(o->xmark - 0x2000, o->ymark, OBJ_MISERY_PHASE)->dir = RIGHT;
      }
      else if (o->timer == 50)
      {
        // switch back to showing real misery instead of the phase-in effect
        o->flags |= FLAG_SHOOTABLE;
        o->invisible = false;
        o->frame     = 0;
        o->dir       = LEFT;

        o->x = o->xmark;
        o->y = o->ymark;

        // spawn rings
        if (o->hp < 340)
        {
          CreateRing(o, 0x00);
          CreateRing(o, 0x80);

          if (o->hp < 180)
          {
            CreateRing(o, 0x40);
            CreateRing(o, 0xC0);
          }
        }

        // after tp we can summon the black balls if the player
        // is far enough away from us that they won't immediately trigger
        if (abs(player->x - o->x) > 112 * CSFI)
        {
          o->state = STATE_SUMMON_BALLS;
        }
        else
        {
          o->state = STATE_FIGHTING;
        }

        // setup sinusoidal hover, both of those possible states
        // are in-air states that do it.
        o->timer    = 0;
        o->yinertia = -0x200;
        // counteracts yinertia of first visible frame, so it's a
        // seamless transition from the phase-in effect.
        o->y += 0x220;
      }
    }
    break;
  }
}

// intro states: stuff that happens before the fight actually starts.
static void run_intro(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      // fixes her position on throne; don't use a spawn point or it'll
      // glitch when she turns to misery_stand in defeated cinematic
      o->y += (6 * CSFI);

      // her initial target height when fight begins
      o->ymark = (64 * CSFI);

      o->state = 1;
    }
    case 1:
    {
      o->frame = 0;
      randblink(o);
    }
    break;

    case 20: // fall from throne (script-triggered)
    {
      o->yinertia += 0x40;

      if (o->blockd)
      {
        o->state = 21;
        o->frame = 2;
      }
    }
    break;
    case 21: // standing/talking after fall from throne
    {
      o->frame = 2;
      randblink(o, 3);
    }
    break;
  }
}

// defeated states--they're all run by the ondeath script.
static void run_defeated(Object *o)
{
  // these states are all script-triggered and must be constant.
  switch (o->state)
  {
    // defeated! "gaah" in air
    case 1000:
    {
      o->flags &= ~FLAG_SHOOTABLE;
      KillObjectsOfType(OBJ_MISERY_RING);
      SmokeClouds(o, 3, 2, 2);

      o->xinertia = 0;
      o->yinertia = 0;

      o->state = 1001;
      o->timer = 0;
      o->frame = 4;
      o->xmark = o->x;
    }
    case 1001: // shake until script tells us otherwise
    {
      o->x = o->xmark;
      if (++o->timer & 2)
        o->x += (1 * CSFI);
    }
    break;

    case 1010: // fall to ground and do defeated frame: "ergh"
    {
      o->yinertia += 10;
      if (o->blockd)
      {
        o->frame = 7;
        o->state = 1011;
      }
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

static Object *CreateRing(Object *o, uint8_t angle)
{
  Object *ring;

  ring               = CreateObject(0, 0, OBJ_MISERY_RING);
  ring->angle        = angle;
  ring->linkedobject = o;

  return ring;
}

void ai_misery_ring(Object *o)
{
  if (!o->linkedobject)
  {
    SmokeClouds(o, 3, 2, 2);
    o->Delete();
    return;
  }

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->timer = 0;
    }
    case 1:
    {
      // distance from misery
      if (o->timer < 192)
        o->timer++;

      // turn to bats when misery teleports
      if (o->linkedobject->state >= STATE_TP_AWAY && o->linkedobject->state < STATE_TP_AWAY + 10)
      {
        o->state = 10;
      }
    }
    break;

    case 10: // transform to bat
    {
      o->flags |= FLAG_SHOOTABLE;
      o->flags &= ~FLAG_INVULNERABLE;

      ThrowObjectAtPlayer(o, 3, 0x200);
      FACEPLAYER;

      o->sprite = SPR_ORANGE_BAT_FINAL;
      o->state  = 11;
    }
    case 11:
    {
      ANIMATE(4, 0, 2);

      if ((o->dir == LEFT && o->blockl) || (o->dir == RIGHT && o->blockr) || o->blocku || o->blockd)
      {
        SmokeClouds(o, 3, 2, 2);
        o->Delete();
      }
    }
    break;
  }
}

void aftermove_misery_ring(Object *o)
{
  if (o->state == 1 && o->linkedobject)
  {
    o->angle += 2;

    int dist = (o->timer * CSFI) / 4;
    o->x     = o->linkedobject->x + xinertia_from_angle(o->angle, dist);
    o->y     = o->linkedobject->y + yinertia_from_angle(o->angle, dist);
  }
}

/*
void c------------------------------() {}
*/

// this is her phasy teleport out/teleport in effect
// it's a 2-dir interlaced picture of her with each dir
// containing only half the lines. We spawn two objects
// in opposite dirs and then separate them.
void ai_misery_phase(Object *o)
{
  XMOVE(0x400);
  if (++o->timer >= 8)
    o->Delete();
}

void ai_misery_ball(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state    = 1;
      o->ymark    = o->y;
      o->xinertia = 0;
      o->yinertia = -0x200;
    }
    case 1:
    {
      ANIMATE(2, 0, 1);

      o->xinertia += (o->x < player->x) ? 0x10 : -0x10;
      o->yinertia += (o->y < o->ymark) ? 0x20 : -0x20;
      LIMITX(0x200);
      LIMITY(0x200);

      if (pdistlx(8 * CSFI) && player->y > o->y)
      {
        o->state = 10;
        o->timer = 0;
      }
    }
    break;

    case 10: // black lightning
    {
      if (++o->timer > 10)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LIGHTNING_STRIKE);
        CreateObject(o->x, o->y, OBJ_BLACK_LIGHTNING);
        o->Delete();
      }

      o->frame = (o->timer & 2) ? 2 : 1;
    }
    break;
  }
}

void ai_black_lightning(Object *o)
{
  ANIMATE(0, 0, 1);
  o->yinertia = 0x1000;

  if (o->blockd)
  {
    effect(o->CenterX(), o->Bottom(), EFFECT_BOOMFLASH);
    SmokeXY(o->CenterX(), o->Bottom(), 3, o->Width() / CSFI, 4);
    o->Delete();
  }
}
