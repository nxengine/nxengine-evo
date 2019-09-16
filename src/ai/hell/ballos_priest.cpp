#include "ballos_priest.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Renderer.h"
#include "../../map.h"
#include "../../player.h"
#include "../../screeneffect.h"
#include "../../sound/SoundManager.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

using namespace NXE::Graphics;

enum STATES
{
  BP_FIGHTING_STANCE = 100, // show fighting stance, then prepare to fly lr

  BP_PREPARE_FLY_LR = 110, // duck a moment, then fly horizontally at player
  BP_PREPARE_FLY_UD = 120, // duck a moment, then fly vertically at player

  BP_FLY_LR   = 130, // flying horizontally
  BP_FLY_UP   = 140, // flying up
  BP_FLY_DOWN = 150, // flying down

  BP_HIT_WALL    = 160, // hit wall while flying horizontally
  BP_HIT_CEILING = 170, // hit ceiling while flying up
  BP_HIT_FLOOR   = 180, // hit floor while flying down

  BP_RETURN_TO_GROUND = 190, // faces screen and floats down to ground
  BP_LIGHTNING_STRIKE = 200, // lightning attack

  BP_DEFEATED = 1000 // defeated (script-triggered)
};

#define DMG_NORMAL 3 // normal damage for touching him
#define DMG_RUSH 10  // damage when he is rushing/flying at you

#define RUSH_SPEED 0x800      // how fast he flies
#define RUSH_DIST (16 * CSFI) // how close he gets to you before changing direction

#define FLOAT_Y MAPY(11)     // Y position to rise to during lightning attack
#define LIGHTNING_Y MAPY(19) // Y position lightning strikes hit (i.e., the floor)

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_BALLOS_PRIEST, ai_ballos_priest);

  ONTICK(OBJ_BALLOS_TARGET, ai_ballos_target);
  ONTICK(OBJ_BALLOS_BONE_SPAWNER, ai_ballos_bone_spawner);
  ONTICK(OBJ_BALLOS_BONE, ai_ballos_bone);
}

/*
void c------------------------------() {}
*/

// defeat sequence
// he flies away, then the script triggers the next form
static void run_defeated(Object *o)
{
  switch (o->state)
  {
    // defeated (script triggered; constant value 1000)
    case BP_DEFEATED:
    {
      o->state++;
      o->timer = 0;
      o->frame = 10;

      o->flags &= ~FLAG_SHOOTABLE;
      effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
      SmokeClouds(o, 16, 16, 16);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BIG_CRASH);

      o->xmark    = o->x;
      o->xinertia = 0;
    }
    case BP_DEFEATED + 1: // fall to ground, shaking
    {
      o->yinertia += 0x20;
      LIMITY(0x5ff);

      o->x = o->xmark;
      if (++o->timer & 2)
        o->x += (1 * CSFI);
      else
        o->x -= (1 * CSFI);

      if (o->blockd && o->yinertia >= 0)
      {
        if (++o->timer > 150)
        {
          o->state++;
          o->timer = 0;
          o->frame = 3;
          FACEPLAYER;
        }
      }
    }
    break;

    case BP_DEFEATED + 2: // prepare to jump
    {
      if (++o->timer > 30)
      {
        o->yinertia = -0xA00;

        o->state++;
        o->frame = 8;
        o->flags |= FLAG_IGNORE_SOLID;
      }
    }
    break;

    case BP_DEFEATED + 3: // jumping
    {
      ANIMATE(1, 8, 9);
      o->dir = LEFT; // up frame

      if (o->y < 0)
      {
        flashscreen.Start();
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);

        o->xinertia = 0;
        o->yinertia = 0;
        o->state++;
      }
    }
    break;
  }
}

// intro cinematic sequence
static void run_intro(Object *o)
{
  switch (o->state)
  {
    // idle/talking to player
    case 0:
    {
      // setup
      o->y -= (6 * CSFI);
      o->dir    = LEFT;
      o->damage = 0;

      // closed eyes/mouth
      o->linkedobject = CreateObject(o->x, o->y - (16 * CSFI), OBJ_BALLOS_SMILE);
      o->state        = 1;
    }
    break;

    // fight begin
    // he smiles, then enters base attack state
    case 10:
    {
      o->timer++;

      // animate smile/open eyes
      if (o->timer > 50)
      {
        Object *smile = o->linkedobject;
        if (smile)
        {
          if (++smile->animtimer > 4)
          {
            smile->animtimer = 0;
            smile->frame++;

            if (smile->frame > 2)
              smile->Delete();
          }
        }

        if (o->timer > 100)
        {
          o->state = BP_FIGHTING_STANCE;
          o->timer = 150;

          o->flags |= FLAG_SHOOTABLE;
          o->flags &= ~FLAG_INVULNERABLE;
        }
      }
    }
    break;
  }
}

// his lightning-strike attack
static void run_lightning(Object *o)
{
  switch (o->state)
  {
    // lightning strikes (targeting player)
    case BP_LIGHTNING_STRIKE:
    {
      o->xmark    = player->x;
      o->yinertia = -0x600;

      o->timer     = 0;
      o->timer2    = 0;
      o->animtimer = 0;

      o->frame = 4;    // facing screen
      o->dir   = LEFT; // not flashing

      o->state++;
    }
    case BP_LIGHTNING_STRIKE + 1:
    {
      ANIMATE(1, 4, 5);

      o->xinertia += (o->x < o->xmark) ? 0x40 : -0x40;
      o->yinertia += (o->y < FLOAT_Y) ? 0x40 : -0x40;
      LIMITX(0x400);
      LIMITY(0x400);

      // run firing
      if (++o->timer > 200)
      {
        int pos = (o->timer % 40);

        if (pos == 1)
        {
          // spawn lightning target
          CreateObject(player->CenterX(), LIGHTNING_Y, OBJ_BALLOS_TARGET)->dir = LEFT;
          o->dir                                                               = RIGHT; // switch to flashing frames
          o->animtimer                                                         = 0;

          // after 8 attacks, switch to even-spaced strikes
          if (++o->timer2 >= 8)
          {
            o->xinertia = 0;
            o->yinertia = 0;

            o->dir       = RIGHT; // flashing
            o->frame     = 5;     // flash red then white during screen flash
            o->animtimer = 1;     // desync animation from screen flashes so it's visible

            o->state++;
            o->timer  = 0;
            o->timer2 = 0;
          }
        }
        else if (pos == 20)
        {
          o->dir = LEFT; // stop flashing
        }
      }
    }
    break;

    // lightning strikes (evenly-spaced everywhere)
    case BP_LIGHTNING_STRIKE + 2:
    {
      ANIMATE(1, 4, 5);
      o->timer++;

      if (o->timer == 40)
        flashscreen.Start();

      if (o->timer > 50)
      {
        if ((o->timer % 10) == 1)
        {
          CreateObject((o->timer2 * TILE_W) * CSFI, LIGHTNING_Y, OBJ_BALLOS_TARGET)->dir = LEFT;
          o->timer2 += 4;

          if (o->timer2 >= 40)
            o->state = BP_RETURN_TO_GROUND;
        }
      }
    }
    break;
  }
}

// creates the two bone spawners which appear when he crashes into the floor or ceiling.
// pass UP if he has hit the ceiling, DOWN if he has hit the floor.
static void spawn_bones(Object *o, int dir)
{
  int y;

  if (dir == UP)
    y = (o->y - (12 * CSFI));
  else
    y = (o->y + (12 * CSFI));

  CreateObject(o->x - (12 * CSFI), y, OBJ_BALLOS_BONE_SPAWNER)->dir = LEFT;
  CreateObject(o->x + (12 * CSFI), y, OBJ_BALLOS_BONE_SPAWNER)->dir = RIGHT;
}

// handles his "looping" flight/rush attacks
static void run_flight(Object *o)
{
  switch (o->state)
  {
    // flying left or right
    case BP_FLY_LR:
    {
      o->state++;
      o->animtimer = 0;
      o->frame     = 6; // flying horizontally

      o->yinertia = 0;
      o->damage   = DMG_RUSH;

      FACEPLAYER;
      XMOVE(RUSH_SPEED);
    }
    case BP_FLY_LR + 1:
    {
      ANIMATE(1, 6, 7);

      // smacked into wall?
      if ((o->blockl && o->dir == LEFT) || (o->blockr && o->dir == RIGHT))
      {
        o->xinertia = 0;
        o->state    = BP_HIT_WALL;
        o->damage   = DMG_NORMAL;
        o->timer    = 0;
        megaquake(10);
      }

      // reached player?
      // this has to be AFTER smacked-into-wall check for proper behavior
      // if player stands in spikes at far left/right of arena.
      if (pdistlx(RUSH_DIST))
        o->state = BP_PREPARE_FLY_UD;
    }
    break;

    // smacked into wall while flying L/R
    case BP_HIT_WALL:
    {
      o->frame = 6;

      if (++o->timer > 30)
      {
        if (o->timer2 <= 3)
          o->state = BP_PREPARE_FLY_LR;
        else
          o->state = BP_RETURN_TO_GROUND;
      }
    }
    break;

    // flying up
    case BP_FLY_UP:
    {
      o->state++;
      o->timer     = 0;
      o->animtimer = 0;

      o->frame = 8;    // vertical flight
      o->dir   = LEFT; // up-facing frame

      o->yinertia = -RUSH_SPEED;
      o->xinertia = 0;
      o->damage   = DMG_RUSH;
    }
    case BP_FLY_UP + 1:
    {
      ANIMATE(1, 8, 9);

      // hit ceiling? (to make this happen, break his loop and jump ABOVE him
      // while he is in the air, at the part where he would normally be
      // coming back down at you).
      if (o->blocku)
      {
        o->state  = BP_HIT_CEILING;
        o->damage = DMG_NORMAL;
        o->timer  = 0;

        SmokeXY(o->CenterX(), o->Top(), 8);
        megaquake(10);

        spawn_bones(o, UP);
      }

      // reached player? (this check here isn't exactly the same as pdistly;
      // it's important that it checks the player's top and not his center).
      if ((abs(player->y - o->y) < RUSH_DIST) && o->timer2 < 4)
        o->state = BP_PREPARE_FLY_LR;
    }
    break;

    case BP_HIT_CEILING: // hit ceiling
    {
      o->frame = 8;

      if (++o->timer > 30)
      {
        if (o->timer2 <= 3)
          o->state = BP_PREPARE_FLY_LR;
        else
          o->state = BP_RETURN_TO_GROUND;
      }
    }
    break;

    // flying down
    case BP_FLY_DOWN:
    {
      o->state++;
      o->timer     = 0;
      o->animtimer = 0;

      o->frame = 8;     // vertical flight
      o->dir   = RIGHT; // down-facing frame

      o->yinertia = RUSH_SPEED;
      o->xinertia = 0;
      o->damage   = DMG_RUSH;
    }
    case BP_FLY_DOWN + 1:
    {
      ANIMATE(1, 8, 9);

      if (o->blockd)
      {
        o->state  = BP_HIT_FLOOR;
        o->damage = DMG_NORMAL;
        o->timer  = 0;

        SmokeXY(o->CenterX(), o->Bottom(), 8);
        megaquake(10);

        spawn_bones(o, DOWN);
        FACEPLAYER;
      }

      if (pdistly(RUSH_DIST) && o->timer2 < 4)
        o->state = BP_PREPARE_FLY_LR;
    }
    break;

    case BP_HIT_FLOOR: // hit floor
    {
      o->frame = 3;

      if (++o->timer > 30)
      {
        o->state = BP_FIGHTING_STANCE;
        o->timer = 120;
      }
    }
    break;

    // come back to ground while facing head on
    case BP_RETURN_TO_GROUND:
    {
      o->frame = 4;    // face screen frame
      o->dir   = LEFT; // non-flashing version

      o->state++;
    }
    case BP_RETURN_TO_GROUND + 1:
    {
      ANIMATE(1, 4, 5);

      o->yinertia += 0x40;
      LIMITY(0x5ff);

      if (o->blockd && o->yinertia >= 0)
      {
        o->state++;
        o->timer = 0;
        o->frame = 3; // landed

        FACEPLAYER;
      }
    }
    break;

    case BP_RETURN_TO_GROUND + 2:
    {
      o->xinertia *= 3;
      o->xinertia /= 4;

      if (++o->timer > 10)
      {
        o->state = BP_FIGHTING_STANCE;
        o->timer = 140;
      }
    }
    break;
  }
}

void ai_ballos_priest(Object *o)
{
  // AIDEBUG;
  // debug("timer3: %d", o->timer3);

  /*if (o->state < 1000)
  {
          FindObjectByID2(500)->Delete();
          game.tsc->StartScript(900);
          return;
  }*/

  run_intro(o);
  run_defeated(o);

  run_flight(o);
  run_lightning(o);

  switch (o->state)
  {
    // show "ninja" stance for "timer" ticks,
    // then prepare to fly horizontally
    case BP_FIGHTING_STANCE:
    {
      o->frame     = 1;
      o->animtimer = 0;
      o->state++;

      o->damage  = DMG_NORMAL;
      o->savedhp = o->hp;
    }
    case BP_FIGHTING_STANCE + 1:
    {
      ANIMATE(10, 1, 2);
      FACEPLAYER;

      if (--o->timer < 0 || (o->savedhp - o->hp) > 50)
      {
        if (++o->timer3 > 4)
        {
          o->state  = BP_LIGHTNING_STRIKE;
          o->timer3 = 0;
        }
        else
        {
          o->state  = BP_PREPARE_FLY_LR;
          o->timer2 = 0;
        }
      }
    }
    break;

    // prepare for flight attack
    case BP_PREPARE_FLY_LR:
    case BP_PREPARE_FLY_UD:
    {
      o->timer2++;
      o->state++;

      o->timer  = 0;
      o->frame  = 3; // fists in
      o->damage = DMG_NORMAL;

      // Fly/UD faces player only once, at start
      FACEPLAYER;
    }
    case BP_PREPARE_FLY_LR + 1:
    {
      FACEPLAYER;
    }
    case BP_PREPARE_FLY_UD + 1:
    {
      // braking, if we came here out of another fly state
      o->xinertia *= 8;
      o->xinertia /= 9;
      o->yinertia *= 8;
      o->yinertia /= 9;

      if (++o->timer > 20)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);

        if (o->state == BP_PREPARE_FLY_LR + 1)
        {
          o->state = BP_FLY_LR; // flying left/right
        }
        else if (player->y < (o->y + (12 * CSFI)))
        {
          o->state = BP_FLY_UP; // flying up
        }
        else
        {
          o->state = BP_FLY_DOWN; // flying down
        }
      }
    }
    break;
  }
}

// targeter for lightning strikes
void ai_ballos_target(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      // position to shoot lightning at passed as x,y
      o->xmark = o->CenterX() - ((Renderer::getInstance()->sprites.sprites[SPR_LIGHTNING].w / 2) * CSFI);
      o->ymark = o->CenterY();

      // adjust our Y coordinate to match player's
      o->y = player->CenterY();

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CHARGE_GUN);
      o->state = 1;
    }
    case 1:
    {
      ANIMATE(1, 0, 1);
      o->timer++;

      if (o->timer == 20 && o->dir == LEFT)
      { // lightning attack
        // setting lightning dir=left: tells it do not flash screen
        CreateObject(o->xmark, o->ymark, OBJ_LIGHTNING)->dir = LEFT;
      }

      if (o->timer > 40)
        o->Delete();
    }
    break;
  }
}

// white sparky thing that moves along floor throwing out bones,
// spawned he hits the ground.
// similar to the red smoke-spawning ones from Undead Core.
void ai_ballos_bone_spawner(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MISSILE_HIT);
      o->state = 1;

      XMOVE(0x400);
    }
    case 1:
    {
      ANIMATE(1, 0, 2);
      o->timer++;

      if ((o->timer % 6) == 1)
      {
        int xi = (random(4, 16) * CSFI) / 8;

        if (o->dir == LEFT)
          xi = -xi;

        CreateObject(o->x, o->y, OBJ_BALLOS_BONE, xi, -0x400);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
      }

      if ((o->blockl && o->xinertia < 0) || (o->blockr && o->xinertia > 0))
      {
        o->Delete();
      }
    }
    break;
  }
}

// bones emitted by bone spawner
void ai_ballos_bone(Object *o)
{
  ANIMATE(3, 0, 2);

  if (o->blockd && o->yinertia >= 0)
  {
    if (o->state == 0)
    {
      o->yinertia = -0x200;
      o->state    = 1;
    }
    else
    {
      effect(o->CenterX(), o->CenterY(), EFFECT_FISHY);
      o->Delete();
    }
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}
