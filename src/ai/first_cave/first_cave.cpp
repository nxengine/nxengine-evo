#include "first_cave.h"

#include "../../common/misc.h"
#include "../../game.h"
#include "../../player.h"
#include "../ai.h"
#include "../sand/puppy.h"
#include "../stdai.h"
#include "../weed/weed.h" // for ai_critter

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_BAT_BLUE, ai_bat_up_down);
  ONTICK(OBJ_CRITTER_HOPPING_BLUE, ai_critter);

  ONTICK(OBJ_HERMIT_GUNSMITH, ai_hermit_gunsmith);
  ONTICK(OBJ_DOOR_ENEMY, ai_door_enemy);
}

/*
void c------------------------------() {}
*/

void ai_bat_up_down(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->ymark = o->y;
      o->timer = random(0, 50);
      o->state = 1;
    case 1:
      if (!o->timer--)
      {
        o->state    = 2;
        o->yinertia = 0x300;
      }
      break;

    case 2:
    {
      if (o->y >= o->ymark)
        o->yinertia -= 0x10;
      else
        o->yinertia += 0x10;

      LIMITY(0x300);
    }
    break;
  }

  FACEPLAYER;
  ANIMATE(1, 2, 4);
}

/*
void c------------------------------() {}
*/

void ai_hermit_gunsmith(Object *o)
{
  if (o->state == 0)
  {
    o->SnapToGround();
    o->state = 1;
  }

  if (o->dir == RIGHT)
  {
    ai_zzzz_spawner(o);
  }
  else
  {
    o->frame = 0;
    randblink(o, 1, 8);
  }
}

void ai_door_enemy(Object *o)
{
  enum
  {
    INIT = 0,
    WAIT,
    OPENEYE,
    CLOSEEYE
  };

  switch (o->state)
  {
    case 0:
      o->state = WAIT;

    case WAIT:
    {
      o->frame = 0;
      if (pdistlx(0x8000) && pdistly(0x8000))
      {
        o->animtimer = 0;
        o->state     = OPENEYE;
      }
    }
    break;

    case OPENEYE:
    {
      if (++o->animtimer > 2)
      {
        o->animtimer = 0;
        o->frame++;
      }

      if (o->frame > 2)
      {
        o->frame = 2;

        if (!pdistlx(0x8000) || !pdistly(0x8000))
        {
          o->state     = CLOSEEYE;
          o->animtimer = 0;
        }
      }
    }
    break;

    case CLOSEEYE:
    {
      if (++o->animtimer > 2)
      {
        o->animtimer = 0;
        if (--o->frame <= 0)
        {
          o->frame = 0;
          o->state = WAIT;
        }
      }
    }
    break;
  }
}
