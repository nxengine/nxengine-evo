#include "ballos_misc.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Tileset.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_BALLOS_SKULL, ai_ballos_skull);
  ONTICK(OBJ_BALLOS_SPIKES, ai_ballos_spikes);

  ONTICK(OBJ_GREEN_DEVIL, ai_green_devil);
  ONTICK(OBJ_GREEN_DEVIL_SPAWNER, ai_green_devil_spawner);

  ONTICK(OBJ_BUTE_SWORD_RED, ai_bute_sword_red);
  ONTICK(OBJ_BUTE_ARCHER_RED, ai_bute_archer_red);

  ONTICK(OBJ_WALL_COLLAPSER, ai_wall_collapser);
}

/*
void c------------------------------() {}
*/

void ai_ballos_skull(Object *o)
{
  ANIMATE(8, 0, 3);

  switch (o->state)
  {
    case 0:
    {
      o->state = 100;
      o->frame = random(0, 16) & 3;
    }
    case 100:
    {
      o->yinertia += 0x40;
      LIMITY(0x700);

      if (o->timer++ & 2)
      {
        (SmokePuff(o->x, o->y))->PushBehind(o);
      }

      if (o->y > 0x10000)
      {
        o->flags &= ~FLAG_IGNORE_SOLID;

        if (o->blockd)
        {
          o->yinertia = -0x200;
          o->state    = 110;
          o->flags |= FLAG_IGNORE_SOLID;

          quake(10, NXE::Sound::SFX::SND_BLOCK_DESTROY);

          for (int i = 0; i < 4; i++)
          {
            Object *s = SmokePuff(o->x + random(-12 * CSFI, 12 * CSFI), o->y + 0x2000);

            s->xinertia = random(-0x155, 0x155);
            s->yinertia = random(-0x600, 0);
            s->PushBehind(o);
          }
        }
      }
    }
    break;

    case 110:
    {
      o->yinertia += 0x40;

      if (o->Top() >= (map.ysize * TILE_H) * CSFI)
      {
        o->Delete();
      }
    }
    break;
  }
}

void ai_ballos_spikes(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (++o->timer < 128)
      {
        o->y -= 0x80;
        o->frame = (o->timer & 2) ? 0 : 1;
      }
      else
      {
        o->state  = 1;
        o->damage = 2;
      }
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

void ai_green_devil_spawner(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->timer = random(0, 40);
      o->state = 1;
    }
    case 1:
    {
      if (--o->timer < 0)
      {
        Object *dv   = CreateObject(o->x, o->y, OBJ_GREEN_DEVIL, 0, 0, o->dir);
        dv->xinertia = random(-16 * CSFI, 16 * CSFI);

        o->state = 0;
      }
    }
    break;
  }
}

void ai_green_devil(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->flags |= FLAG_SHOOTABLE;
      o->ymark    = o->y;
      o->yinertia = random(-5 * CSFI, 5 * CSFI);
      o->damage   = 3;
      o->state    = 1;
    }
    case 1:
    {
      ANIMATE(2, 0, 1);
      o->yinertia += (o->y < o->ymark) ? 0x80 : -0x80;

      XACCEL(0x20);
      LIMITX(0x400);

      if (o->dir == LEFT)
      {
        if (o->x < -o->Width())
          o->Delete();
      }
      else
      {
        if (o->x > ((map.xsize * TILE_W) * CSFI) + o->Width())
          o->Delete();
      }
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

void ai_bute_sword_red(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state  = 1;
      o->sprite = SPR_BUTE_SWORD_RED_FALLING;
      o->MoveAtDir(o->dir, 0x600);
      o->dir = 0;
    }
    case 1:
    {
      ANIMATE(2, 0, 3);

      if (++o->timer == 8)
        o->flags &= ~FLAG_IGNORE_SOLID;

      if (o->timer >= 16)
      {
        o->state  = 10;
        o->sprite = SPR_BUTE_SWORD_RED;
        o->frame  = 0;

        o->flags |= FLAG_SHOOTABLE;
        o->damage = 5;
      }
    }
    break;

    case 10:
    {
      ANIMATE(1, 0, 1);
      FACEPLAYER;

      // when player is below them, they come towards him,
      // when player is above, they sweep away.
      if (player->CenterY() > (o->y + (24 * CSFI)))
      {
        XACCEL(0x10);
      }
      else
      {
        XACCEL(-0x10);
      }

      o->yinertia += (o->y <= player->y) ? 0x10 : -0x10;

      if ((o->blockl && o->xinertia < 0) || (o->blockr && o->xinertia > 0))
      {
        o->xinertia = -o->xinertia;
      }

      if ((o->blocku && o->yinertia <= 0) || (o->blockd && o->yinertia >= 0))
      {
        o->yinertia = -o->yinertia;
      }

      LIMITX(0x5ff);
      LIMITY(0x5ff);
    }
    break;
  }
}

void ai_bute_archer_red(Object *o)
{
  // DebugCrosshair(o->x, o->y, 0, 255, 255);

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;

      o->xmark = o->x;
      o->ymark = o->y;

      if (o->dir == LEFT)
        o->xmark -= (128 * CSFI);
      else
        o->xmark += (128 * CSFI);

      o->xinertia = random(-0x400, 0x400);
      o->yinertia = random(-0x400, 0x400);
    }
    case 1: // come on screen
    {
      ANIMATE(1, 0, 1);

      if ((o->dir == LEFT && o->x < o->xmark) || (o->dir == RIGHT && o->x > o->xmark))
      {
        o->state = 20;
      }
    }
    break;

    case 20: // aiming
    {
      o->state = 21;
      o->timer = random(0, 150);

      o->frame     = 2;
      o->animtimer = 0;
    }
    case 21:
    {
      ANIMATE(2, 2, 3);

      if (++o->timer > 300 || (pdistlx(112 * CSFI) && pdistly(16 * CSFI)))
      {
        o->state = 30;
      }
    }
    break;

    case 30: // flashing to fire
    {
      o->state     = 31;
      o->timer     = 0;
      o->animtimer = 0;
      o->frame     = 3;
    }
    case 31:
    {
      ANIMATE(1, 3, 4);

      if (++o->timer > 30)
      {
        o->state = 40;
        o->frame = 5;

        Object *arrow   = CreateObject(o->x, o->y, OBJ_BUTE_ARROW);
        arrow->dir      = o->dir;
        arrow->xinertia = (o->dir == RIGHT) ? 0x800 : -0x800;
      }
    }
    break;

    case 40: // fired
    {
      o->state     = 41;
      o->timer     = 0;
      o->animtimer = 0;
    }
    case 41:
    {
      ANIMATE(2, 5, 6);

      if (++o->timer > 40)
      {
        o->state    = 50;
        o->timer    = 0;
        o->xinertia = 0;
        o->yinertia = 0;
      }
    }
    break;

    case 50: // retreat offscreen
    {
      ANIMATE(1, 0, 1);
      XACCEL(-0x20);

      if (o->Right() < 0 || o->Left() > ((map.xsize * TILE_W) * CSFI))
        o->Delete();
    }
    break;
  }

  // sinusoidal hover around set point
  if (o->state != 50)
  {
    o->xinertia += (o->x < o->xmark) ? 0x2A : -0x2A;
    o->yinertia += (o->y < o->ymark) ? 0x2A : -0x2A;
    LIMITX(0x400);
    LIMITY(0x400);
  }
}

/*
void c------------------------------() {}
*/

// This object is responsible for collapsing the walls in the final best-ending sequence.
// All the original object does is collapse one tile further every 101 frames.
// However, since it's triggered at the beginning of the cinematic and then is let to run
// through almost the entire thing it needs to be sync'd really-really perfect with a
// number of other systems; the textboxes, etc.
//
// I spent several hours trying to get my events to run in perfect frame-by-frame
// exactness with the original engine, and found several things that were slightly off.
// However, I've decided that even if I got it absolutely perfect, it's too liable to
// get broken by some minor innocent change in the future, and requires too much of
// the engine to be tuned just so.
//
// So, I've added some event-based triggers to the object, that are NOT technically supposed
// to be there. These will make extra sure that nothing embarrassing happens during this great
// finale, such as the walls being one tile too far at one point, or even worse, having
// them collapse onto Balrog before he makes it to the exit. Because there are no triggers
// in the script and I can't change the script, I had to do a bit of sneaky spying on program
// state to implement them.
void ai_wall_collapser(Object *o)
{
  int y;

  switch (o->state)
  {
    case 0:
    {
      o->invisible = true;
      o->timer     = 0;
      o->state     = 1;
    }
    break;

    case 10: // trigger
    {
      if (++o->timer > 100)
      {
        o->timer2++;
        o->timer = 0;

        int xa = (o->x / CSFI) / TILE_W;
        int ya = (o->y / CSFI) / TILE_H;
        for (y = 0; y < 20; y++)
        {
          // pushing the smoke behind all objects prevents it from covering
          // up the NPC's on the collapse just before takeoff.
          map_ChangeTileWithSmoke(xa, ya + y, 109, 4, false, lowestobject);
        }

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
        quake(20);

        if (o->dir == LEFT)
          o->x -= (TILE_W * CSFI);
        else
          o->x += (TILE_W * CSFI);

        // reached the solid tile in the center of the throne.
        // it isn't supposed to cover this tile until after Curly
        // says we're gonna get crushed.
        if (o->timer2 == 6)
          o->state = 20;

        // balrog is about to take off/rescue you.
        if (o->timer2 == 9)
          o->state = 30;
      }
    }
    break;

    // "gonna get crushed" event
    case 20:
    {
      // wait for text to come up
      if (textbox.IsVisible())
        o->state = 21;
    }
    break;
    case 21:
    {
      // wait for text to dismiss, then tile immediately collapses
      if (!textbox.IsVisible())
      {
        o->state = 10;
        o->timer = 1000;
      }
    }
    break;

    // balrog is about to take off. the video I took shows that
    // the walls are supposed to collapse into your space on the
    // exact same frame that he breaks the first ceiling tile.
    case 30:
    {
      o->linkedobject = Objects::FindByType(OBJ_BALROG_DROP_IN);
      if (o->linkedobject)
        o->state = 31;
    }
    break;
    case 31:
    {
      // debug("%x", o->linkedobject->y);
      if (o->linkedobject && o->linkedobject->y <= 0x45800)
      {
        o->state = 10;
        o->timer = 1000;
      }
    }
    break;
  }
}
