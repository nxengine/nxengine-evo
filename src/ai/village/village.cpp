#include "village.h"

#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/tileset.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../ai.h"
#include "../stdai.h"
#include "../weed/weed.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_TOROKO_SHACK, ai_toroko_shack);

  ONTICK(OBJ_MUSHROOM_ENEMY, ai_mushroom_enemy);
  ONTICK(OBJ_GIANT_MUSHROOM_ENEMY, ai_mushroom_enemy);
  ONTICK(OBJ_GRAVEKEEPER, ai_gravekeeper);

  ONTICK(OBJ_CAGE, ai_cage);
  ONSPAWN(OBJ_FLOWERS_PENS1, onspawn_snap_to_ground);
}

/*
void c------------------------------() {}
*/

// scared toroko attacking in Shack
void ai_toroko_shack(Object *o)
{
  // debug("Y:%d  Yinertia:%d  blockd:%d  flags:%d", o->y/CSFI, o->yinertia, o->blockd, o->flags);

  switch (o->state)
  {
    case 0:
      o->timer    = 0;
      o->yinertia = -(2 * CSFI);
      o->y -= (2 * CSFI);
      o->flags |= FLAG_IGNORE_SOLID;
      o->state = 1;
    case 1:
    {
      ANIMATE(2, 0, 3);
      XMOVE(0x100);

      if (o->yinertia >= 0)
      {
        o->flags &= ~FLAG_IGNORE_SOLID;

        if (o->blockd)
        {
          ASSERT((o->y % TILE_H) == 0);

          o->yinertia = 0;
          o->state    = 2;
        }
      }
    }
    break;

    case 2:
      o->frame = o->timer = o->animtimer = 0;
      o->state                           = 3;
    case 3:
    {
      ANIMATE(2, 0, 3);
      XACCEL(0x40);

      if (++o->timer >= 50)
      {
        o->timer    = 40;
        o->xinertia = -o->xinertia;
        o->dir      = (o->dir == RIGHT) ? LEFT : RIGHT;
      }

      if (o->timer >= 35)
        o->flags |= FLAG_SHOOTABLE;

      if (o->shaketime)
      {
        o->state    = 4;
        o->frame    = 4;
        o->yinertia = -(2 * CSFI);
        o->flags &= ~FLAG_SHOOTABLE;
        o->damage = 0;
        o->timer  = 0;
      }
    }
    break;

    case 4:
      XMOVE(0x100);

      if (++o->timer > 2 && o->blockd)
      {
        o->state = 5;
        o->flags |= FLAG_SCRIPTONACTIVATE;
      }
      break;

    case 5:
      o->xinertia = 0;
      o->frame    = 5;
      break;
  }

  o->yinertia += 0x40;
  LIMITX(0x400);
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

// mushroom enemy ("Pignon") from Mimiga Graveyard
void ai_mushroom_enemy(Object *o)
{
  enum
  {
    INIT = 0,
    STANDING,
    BLINKING,
    WALKING,
    WALKING2,
    SHOT
  };

  switch (o->state)
  {
    case 0:
      o->frame     = 0;
      o->animtimer = 0;
      o->xinertia  = 0;
      o->state     = STANDING;
    case STANDING: // stand around
    {
      if (!random(0, 100))
      { // blink
        o->state = BLINKING;
        o->frame = 1;
        o->timer = 0;
      }
      else
      {
        if (!random(0, 150))
          o->dir ^= 1;

        // start walking
        if (!random(0, 150))
        {
          o->state = WALKING;
          o->frame = 0;
          o->timer = 50;
        }
      }
    }
    break;

    case BLINKING: // eyes closed/blinking
    {
      if (++o->timer > 8)
      {
        o->frame = 0;
        o->state = STANDING;
      }
    }
    break;

    case WALKING: // walking
      o->state++;
      o->frame     = 2;
      o->animtimer = 0;
    case WALKING + 1:
    {
      if (!--o->timer)
        o->state = INIT;

      if (o->blockl && o->dir == LEFT)
      {
        o->dir      = RIGHT;
        o->xinertia = 0x200;
      }
      else if (o->blockr && o->dir == RIGHT)
      {
        o->dir      = LEFT;
        o->xinertia = -0x200;
      }

      ANIMATE(2, 2, 4);
      XMOVE(0x100);
    }
    break;

    case SHOT: // hit by shot
      if (o->blockd)
        o->state = INIT;
      break;
  }

  if (o->shaketime && o->state != SHOT)
  {
    o->state    = SHOT;
    o->yinertia = -0x200;
    o->frame    = 6;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

// guy with knife, Mimiga Graveyard
void ai_gravekeeper(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->flags |= FLAG_SHOOTABLE;
      o->flags |= FLAG_INVULNERABLE;
      o->nxflags |= NXFLAG_FOLLOW_SLOPE;

      o->damage = 0;
      o->state  = 1;
    case 1:
    {
      o->frame = 0;
      FACEPLAYER;

      // start walking when player comes near
      if (pdistlx(128 * CSFI) && pdistly2(48 * CSFI, 32 * CSFI))
      {
        o->state     = 2;
        o->animtimer = 0;
      }

      // start walking if shot
      if (o->shaketime)
      {
        o->state     = 2;
        o->frame     = 1;
        o->animtimer = 0;
        o->flags |= FLAG_INVULNERABLE;
      }
    }
    break;

    case 2: // walking
    {
      ANIMATE(6, 0, 3);

      // reached knife range of player?
      // (no, he doesn't check Y)
      if (pdistlx(10 * CSFI))
      {
        o->state = 3;
        o->timer = 0;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);
        o->flags &= ~FLAG_INVULNERABLE;
      }

      FACEPLAYER;
      XMOVE(0x100);
    }
    break;

    case 3: // knife raised
    {
      o->frame    = 4;
      o->xinertia = 0;

      if (++o->timer > 40)
      {
        o->state = 4;
        o->timer = 0;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SLASH);
      }
    }
    break;

    case 4: // knife frame 2
    {
      o->frame  = 5;
      o->damage = 10;

      if (++o->timer > 2)
      {
        o->state = 5;
        o->timer = 0;
      }
    }
    break;

    case 5: // knife frame 3
    {
      o->frame = 6;
      if (++o->timer > 60)
        o->state = 0;

      if (o->xinertia > 0 && o->blockl)
        o->xinertia = 0;
      if (o->xinertia < 0 && o->blockr)
        o->xinertia = 0;
    }
    break;
  }

  o->yinertia += 0x20;
  LIMITX(0x400);
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_cage(Object *o)
{
  if (o->state == 0)
  {
    // the Cage's drawpoint is used to tweak the positioning
    // of the camera, which is <FON'd on it, during the post-Egg cutscene,
    // see the rules for draw points in map_scroll_do().
    o->x += o->DrawPointX();
    o->y += o->DrawPointY();
    o->state = 1;
  }
}
