#include "balcony.h"

#include "../../ObjManager.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/graphics.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"
using namespace Graphics;
#include "../../autogen/sprites.h"
#include "../../graphics/tileset.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_HELICOPTER, ai_helicopter);
  ONTICK(OBJ_HELICOPTER_BLADE, ai_helicopter_blade);

  ONTICK(OBJ_IGOR_BALCONY, ai_igor_balcony);

  ONTICK(OBJ_FALLING_BLOCK, ai_falling_block);
  ONTICK(OBJ_FALLING_BLOCK_SPAWNER, ai_falling_block_spawner);
}

/*
void c------------------------------() {}
*/

void ai_helicopter(Object *o)
{
  Object *b;

  switch (o->state)
  {
    case 0: // stopped
    {
      b         = CreateObject(o->x + (26 * CSFI) - (56 * CSFI), o->y - (9 * CSFI) - (48 * CSFI), OBJ_HELICOPTER_BLADE);
      b->sprite = SPR_HELICOPTER_BLADE_1;
      b->linkedobject = o;

      b         = CreateObject(o->x - (4 * CSFI) - (56 * CSFI), o->y - (4 * CSFI) - (48 * CSFI), OBJ_HELICOPTER_BLADE);
      b->sprite = SPR_HELICOPTER_BLADE_2;
      b->linkedobject = o;

      o->state = 1;
    }
    break;

    case 20: // blades running
      break;

    case 30: // blades running, spawn momorin
    {
      CreateObject(o->x + (45 * CSFI) - (56 * CSFI), o->y + (34 * CSFI) - (48 * CSFI), OBJ_MOMORIN)->dir = LEFT;

      o->dir   = RIGHT; // open hatch
      o->state = 31;
    }
    break;

    case 40: // blades running, spawn momorin, santa, and chako (from credits)
    {
      CreateObject(o->x + (47 * CSFI) - (56 * CSFI), o->y + (34 * CSFI) - (48 * CSFI), OBJ_MOMORIN)->dir = LEFT;
      CreateObject(o->x + (34 * CSFI) - (56 * CSFI), o->y + (34 * CSFI) - (48 * CSFI), OBJ_SANTA)->dir   = LEFT;
      CreateObject(o->x + (21 * CSFI) - (56 * CSFI), o->y + (34 * CSFI) - (48 * CSFI), OBJ_CHACO)->dir   = LEFT;

      o->dir   = RIGHT; // open hatch
      o->state = 41;
    }
    break;
  }
}

void ai_helicopter_blade(Object *o)
{
  switch (o->state)
  {
    case 0:
    case 1:
    {
      if (o->linkedobject && o->linkedobject->state >= 20)
        o->state = 10;
    }
    break;

    case 10:
    {
      ANIMATE(0, 0, 3);
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

void ai_igor_balcony(Object *o)
{
  if (!pdistlx(SCREEN_WIDTH * CSFI) && !pdistly(SCREEN_HEIGHT * CSFI))
  {
    o->state = 1;
  }

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
    }
    case 1:
    {
      ANIMATE(20, 0, 1);

      if ((pdistlx(112 * CSFI) && pdistly2(48 * CSFI, 112 * CSFI)) || o->shaketime)
      {
        o->state = 10;
      }
    }
    break;

    case 10: // walking towards player
    {
      o->state     = 11;
      o->frame     = 0;
      o->animtimer = 0;
      FACEPLAYER;
    }
    case 11:
    {
      ANIMATE(4, 2, 5);
      XMOVE(0x200);

      if ((o->dir == RIGHT && o->blockr) || (o->dir == LEFT && o->blockl) || pdistlx(64 * CSFI))
      {
        o->xinertia = 0;
        o->state    = 20;
        o->timer    = 0;
      }
    }
    break;

    case 20: // prepare to jump...
    {
      o->frame = 11; // jump-prepare frame

      if (++o->timer > 10)
      {
        o->state    = 21;
        o->yinertia = -0x5ff;
        XMOVE(0x200);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_IGOR_JUMP);
      }
    }
    break;

    case 21: // jumping
    {
      o->frame = 10; // in-air frame

      if (o->blockd && o->yinertia >= 0)
      {
        quake(20);
        o->xinertia = 0;

        o->state = 22;
        o->timer = 0;
        o->frame = 11;
      }
    }
    break;

    case 22: // landed
    {
      if (++o->timer > 30)
        o->state = 30;
    }
    break;

    case 30: // mouth-blast attack
    {
      o->state = 31;
      o->timer = 0;
      FACEPLAYER;
    }
    case 31:
    {
      o->timer++;

      // flash mouth
      o->frame = 8;
      if (o->timer < 50 && (o->timer & 2))
        o->frame = 9;

      // fire shots
      if (o->timer > 30)
      {
        if ((o->timer % 4) == 1)
        {
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
          Object *shot = SpawnObjectAtActionPoint(o, OBJ_IGOR_SHOT);

          int angle = (o->dir == LEFT) ? 136 : 248;
          angle += random(-16, 16);
          ThrowObjectAtAngle(shot, angle, 0x580);
        }
      }

      if (o->timer > 82)
      {
        FACEPLAYER;
        o->state = 10;
      }
    }
    break;
  }

  o->yinertia += 0x33;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_falling_block_spawner(Object *o)
{
  switch (o->state)
  {
    // wait till player leaves "safe zone" at start of Balcony
    // does nothing in Hell--you enter from the left.
    case 0:
    {
      if (player->x < MAPX(map.xsize - 6))
      {
        o->state = 1;
        o->timer = 24;
      }
    }
    break;

    case 1:
    {
      if (--o->timer < 0)
      {
        Object *block;
        int x;

        // blocks tend to follow behind the player--this goes along
        // with the text that tells you to run so as not to get squashed.
        if (player->equipmask & EQUIP_BOOSTER20)
        {
          x = (player->x + MAPX(4));
          if (x < MAPX(26))
            x = MAPX(26);
        }
        else
        {
          x = (player->x + MAPX(6));
          if (x < MAPX(23))
            x = MAPX(23);
        }

        if (x > MAPX(map.xsize - 10))
          x = MAPX(map.xsize - 10);

        if (player->equipmask & EQUIP_BOOSTER20)
        {
          x += ((random(-14, 14) * TILE_W) * CSFI);
        }
        else
        {
          x += ((random(-11, 11) * TILE_H) * CSFI);
        }

        block      = CreateObject(x, (player->y - MAPY(14)), OBJ_FALLING_BLOCK);
        block->dir = random(0, 1) ? LEFT : RIGHT; // small/big blocks

        o->timer = random(9, 24);
      }
    }
    break;
  }
}

void ai_falling_block(Object *o)
{
  o->damage = (player->Top() > o->CenterY()) ? 10 : 0;
  // debug("Falling Block State %d", o->state);

  switch (o->state)
  {
    case 0:
    {
      bool in_hell = (game.curmap == STAGE_HELL2);

      switch (o->dir)
      {
        case LEFT: // large Hell or Balcony block
        {
          o->sprite = (in_hell) ? SPR_HELL_BLOCK_LARGE : SPR_BALCONY_BLOCK_LARGE;
          o->flags |= (FLAG_INVULNERABLE | FLAG_IGNORE_SOLID);
          o->state = 10;
        }
        break;

        case RIGHT: // small Hell or Balcony block
        {
          o->sprite = (in_hell) ? SPR_HELL_BLOCK_SMALL : SPR_BALCONY_BLOCK_SMALL;
          o->flags |= (FLAG_INVULNERABLE | FLAG_IGNORE_SOLID);
          o->state = 10;
        }
        break;

        case DOWN: // Misery-spawned block
        {
          o->state = 1;
          o->timer = 0;
        }
        break;
      }

      o->dir = 0; // there is actually only one dir on sprite
    }
    break;

    case 1: // just spawned by Misery--pause a moment
    {
      if (++o->timer > 3)
      {
        o->flags |= FLAG_INVULNERABLE;
        o->state = 10;
      }
    }
    break;

    case 10: // falling
    {        // allow to pass thru Hell/Balcony ceiling
      if (o->y > 128 * CSFI)
      {
        o->flags &= ~FLAG_IGNORE_SOLID;
        o->state = 11;
      }
    }
    case 11: // passed thru ceiling in Hell B2
    {
      o->yinertia += 0x40;
      LIMITY(0x700);

      if (o->blockd)
      {
        o->yinertia = -0x200;
        o->flags |= FLAG_IGNORE_SOLID;
        o->state = 20;

        SmokeSide(o, 4, DOWN);
        quake(10);
      }
    }
    break;

    case 20: // already bounced on ground, falling offscreen
    {
      o->yinertia += 0x40;
      LIMITY(0x700);

      if (o->y > ((map.ysize * TILE_H) * CSFI))
      {
        o->Delete();
      }
    }
    break;
  }
}
