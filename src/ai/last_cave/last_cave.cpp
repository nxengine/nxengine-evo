#include "last_cave.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Renderer.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../plantation/plantation.h" // ai_droll_shot
#include "../stdai.h"
#include "../sym/smoke.h"
#include "../sym/sym.h" // ai_press

using namespace NXE::Graphics;

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_CRITTER_HOPPING_RED, ai_critter_hopping_red);

  ONTICK(OBJ_LAVA_DRIP_SPAWNER, ai_lava_drip_spawner);
  ONTICK(OBJ_LAVA_DRIP, ai_lava_drip);

  ONTICK(OBJ_RED_BAT_SPAWNER, ai_red_bat_spawner);
  ONTICK(OBJ_RED_BAT, ai_red_bat);

  ONTICK(OBJ_RED_DEMON, ai_red_demon);
  ONTICK(OBJ_RED_DEMON_SHOT, ai_droll_shot);

  ONTICK(OBJ_PROXIMITY_PRESS_VERT, ai_proximity_press_vert);
}

/*
void c------------------------------() {}
*/

void ai_proximity_press_vert(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (pdistlx(8 * CSFI) && pdistly2(8 * CSFI, 128 * CSFI) && !o->blockd)
      {
        o->state     = 10;
        o->animtimer = 0;
        o->frame     = 1;
      }
    }
    break;

    case 10:
    {
      if (o->frame < 2)
        ANIMATE_FWD(2);

      if (o->blockd)
      {
        if (o->frame >= 2) // make sure eye fully open
        {
          SmokeSide(o, 4, DOWN);
          quake(10);
        }

        o->flags |= FLAG_SOLID_BRICK;
        o->damage = 0;

        o->state = 11;
        o->frame = 0;
      }
      else
      {
        if (player->Top() > o->CenterY())
        {
          o->flags &= ~FLAG_SOLID_BRICK;
          o->damage = 127;
        }
        else
        {
          o->flags |= FLAG_SOLID_BRICK;
          o->damage = 0;
        }
      }
    }
    break;
  }

  if (o->state >= 5)
  {
    o->yinertia += 0x80;
    LIMITY(0x5ff);
  }
}

/*
void c------------------------------() {}
*/

void ai_critter_hopping_red(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      FACEPLAYER;
      o->frame = 0;

      if (o->shaketime)
      {
        o->state = 1;
        o->timer = 0;
      }
      else if (++o->timer >= 8)
      {
        if (pdistly((5 * TILE_H) * CSFI))
        {
          if (pdistlx((6 * TILE_W) * CSFI))
          {
            o->state = 1;
            o->timer = 0;
          }
          else if (pdistlx((9 * TILE_W) * CSFI))
          {
            o->frame = 1;
          }
        }
      }
    }
    break;

    case 1: // prepare to jump
    {
      if (++o->timer > 8)
      {
        o->state = 2;
        o->frame = 2;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);

        o->yinertia = -0x5ff;
        o->xinertia = (o->dir == RIGHT) ? 0x200 : -0x200;
      }
    }
    break;

    case 2: // in air
    {
      if (o->blockd && o->yinertia > 0)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
        o->xinertia = 0;

        o->state = 0;
        o->timer = 0;
        o->frame = 1;
      }
    }
    break;
  }

  o->yinertia += 0x55;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

// lava drip spawner in Last Cave (hidden) and used extensively in Final Cave.
// the id1 tag sets the frequency of the drip, the id2 tag sets an amount to
// delay the first drip in order to desync a group of drips.
void ai_lava_drip_spawner(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->sprite = SPR_LAVA_DRIP;
      o->x += (4 * CSFI);
      o->timer = (o->id2 - o->id1);
      o->state = 1;
    }
    case 1:
    {
      if (--o->timer < 0)
      {
        o->state     = 2;
        o->animtimer = 0;
        o->timer2    = 0;
      }
    }
    break;

    case 2:
    {
      o->display_xoff = (++o->timer2 & 2) ? 0 : 1;

      ANIMATE_FWD(10);
      if (o->frame > 3)
      {
        o->frame = 0;
        o->state = 1;
        o->timer = o->id1;

        ai_lava_drip(CreateObject(o->x, o->y, OBJ_LAVA_DRIP));
      }
    }
    break;
  }
}

void ai_lava_drip(Object *o)
{
  o->frame = 4;
  o->yinertia += 0x40;
  LIMITY(0x5ff);

  if (o->blockd || (++o->timer > 10 && o->CheckAttribute(&Renderer::getInstance()->sprites.sprites[o->sprite].block_u, TA_WATER)))
  {
    for (int i = 0; i < 3; i++)
    {
      Caret *c    = effect(o->CenterX(), o->Bottom(), EFFECT_LAVA_SPLASH);
      c->xinertia = random(-0x400, 0x400);
      c->yinertia = random(-0x400, 0);
    }

    if (o->onscreen)
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BUBBLE);

    o->Delete();
  }
}

/*
void c------------------------------() {}
*/

void ai_red_bat_spawner(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->timer = random(0, 500);
    }
    case 1:
    {
      if (--o->timer < 0)
      {
        Object *bat = CreateObject(o->CenterX(), o->CenterY() + random(-32 * CSFI, 32 * CSFI), OBJ_RED_BAT);
        bat->x -= (bat->Width() / 2);
        bat->y -= (bat->Height() / 2);
        bat->dir = o->dir;
        o->state = 0;
      }
    }
  }
}

void ai_red_bat(Object *o)
{
  ANIMATE(1, 0, 2);

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->ymark = o->y;
      o->timer = random(0, 50);
    }
    case 1:
    {
      if (--o->timer < 0)
      {
        o->state    = 2;
        o->yinertia = 0x400;
      }
      else
        break;
    }
    case 2:
    {
      o->yinertia += (o->y < o->ymark) ? 0x10 : -0x10;
      LIMITY(0x300);
      XMOVE(0x100);
    }
    break;
  }

  if (o->x < 0 || o->x > (map.xsize * TILE_W) * CSFI)
  {
    effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
    o->Delete();
  }
}

/*
void c------------------------------() {}
*/

void ai_red_demon(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->xmark    = o->x;
      o->xinertia = 0;
      o->frame    = 0;
      o->state    = 1;
    }
    case 1:
    {
      ANIMATE(20, 0, 1);
      FACEPLAYER;
    }
    break;

    case 10: // prepare to jump
    {
      o->flags |= FLAG_SHOOTABLE;
      o->state = 11;
      o->frame = 3;
      o->timer = 0;
    }
    case 11:
    {
      switch (++o->timer)
      {
        case 30:
        case 40:
        case 50:
        {
          o->frame = 4;
          EmFireAngledShot(o, OBJ_RED_DEMON_SHOT, 0, 0x800);
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
        }
        break;

        case 34:
        case 44:
        case 54:
        {
          o->frame = 3;
        }
        break;

        case 61:
        {
          o->state = 20;
          o->timer = 0;
          o->frame = 2;
        }
        break;
      }
    }
    break;

    case 20: // pause before jump
    {
      if (++o->timer > 20)
      {
        o->state = 21;
        o->timer = 0;
        o->frame = 5;

        o->yinertia = -0x5ff;
        o->xinertia = (o->CenterX() < player->CenterX()) ? 0x100 : -0x100;
      }
    }
    break;

    case 21: // in air
    {
      switch (++o->timer)
      {
        case 30:
        case 40:
        case 50:
        {
          o->frame = 6;
          EmFireAngledShot(o, OBJ_RED_DEMON_SHOT, 0, 0x800);
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
        }
        break;

        case 34:
        case 44:
        {
          o->frame = 5;
        }
        break;

        case 54:
        {
          o->frame = 7;
        }
        break;
      }

      if (o->blockd && o->yinertia >= 0)
      {
        quake(10);
        o->state = 22;
        o->timer = 0;
        o->frame = 2;
      }
    }
    break;

    case 22: // landed
    {
      o->xinertia /= 2;

      if (++o->timer > 22)
      {
        o->state = 10;
      }
    }
    break;

    // defeated/turned to stone (set by script)
    case 50:
    {
      o->flags &= ~FLAG_SHOOTABLE;
      o->damage = 0;

      if (o->blockd)
      {
        o->state = 51;
        o->frame = 2;

        game.quaketime = 10;
        SmokeClouds(o, 12, 4, 4);
        o->SpawnXP(19);

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BIG_CRASH);

        // needed to prevent status bars from not disappearing
        game.bossbar.object = NULL;
      }
    }
    break;

    case 51:
    {
      o->xinertia *= 7;
      o->xinertia /= 8;

      o->frame = 8;
    }
    break;
  }

  if (o->state < 50)
  {
    FACEPLAYER;
  }

  o->yinertia += 0x20;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_press_vert(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->y -= (4 * CSFI);

      if (pdistlx((8 * CSFI)) && pdistly2((8 * CSFI), (128 * CSFI)))
      {
        o->state = 5;
      }
    }
    break;

    case 5:
    {
      if (o->blockd)
      {
        o->state     = 10;
        o->animtimer = 0;
        o->frame     = 1;
      }
    }
    break;

    case 10:
    {
      ANIMATE_FWD(2);
      if (o->frame > 2)
        o->frame = 2;

      if (player->y > o->y)
        o->flags |= FLAG_SOLID_BRICK;
    }
    break;
  }
}
