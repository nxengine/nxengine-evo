#include "misery.h"

#include "../../ObjManager.h"
#include "../../common/misc.h"
#include "../../common/stat.h"
#include "../../game.h"
#include "../../map.h"
#include "../../screeneffect.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_MISERY_FLOAT, ai_misery_float);
  ONTICK(OBJ_MISERY_STAND, ai_misery_stand);
  ONTICK(OBJ_MISERYS_BUBBLE, ai_miserys_bubble);
}

/*
void c------------------------------() {}
*/

// teleport in and float
void ai_misery_float(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state = 1;
      o->x += (1 * CSFI); // check Undead Core intro to prove this is correct
      o->xmark = o->x;
      o->ymark = o->y;
      o->frame = 0;
      o->timer = 0;
    case 1:
      if (DoTeleportIn(o, 1))
        o->state = 10;
      break;

    case 10: // floating
      o->state     = 11;
      o->timer     = 0;
      o->animframe = 0;
      o->yinertia  = (1 * CSFI);
    case 11:
      if (o->y > o->ymark)
        o->yinertia -= 16;
      if (o->y < o->ymark)
        o->yinertia += 16;
      if (o->yinertia > 0x100)
        o->yinertia = 0x100;
      if (o->yinertia < -0x100)
        o->yinertia = -0x100;
      break;

    case 13: // fall from floaty
      o->frame = 1;
      o->flags &= ~FLAG_IGNORE_SOLID;

      o->yinertia += 0x40;
      if (o->yinertia > 0x5ff)
        o->yinertia = 0x5ff;

      if (o->blockd)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
        o->yinertia = 0;
        o->state    = 14;
        o->flags |= FLAG_IGNORE_SOLID;
        o->animframe = 2;
        o->animtimer = 30; // blink as soon as hit ground
      }
      break;

    case 14:
      break; // standing

    // spawn the bubble which picks up Toroko in Shack
    case 15:
      o->frame = 4;
      o->timer = 0;
      o->state = 16;
    case 16:
    {
      o->timer++;

      if (o->timer == 30)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BUBBLE);
        CreateObject(o->x, o->y - (16 * CSFI), OBJ_MISERYS_BUBBLE);
      }

      if (o->timer >= 50)
        o->state = 14;
    }
    break;

    case 20: // fly away
      o->state    = 21;
      o->frame    = 0;
      o->yinertia = 0;
      o->flags |= FLAG_IGNORE_SOLID;
    case 21:
      o->yinertia -= 0x20;
      if (o->y < -0x1000)
        o->Delete();
      break;

    case 25: // big spell
    case 26: // she flashes, then a clap of thunder
      ai_misery_stand(o);
      break;
    case 27: // return to standing after lightning strike
      if (++o->timer > 16)
        o->state = 14;
      break;
  }

  if (o->state == 11 || o->state == 14)
  {
    // blink
    if (o->animtimer)
    {
      o->animtimer--;
      o->frame = 1;
    }
    else
    {
      o->frame = 0;
      if (random(0, 100) == 1)
      {
        o->animtimer = 30;
      }
    }

    o->frame += o->animframe;
  }
}

void ai_miserys_bubble(Object *o)
{
  Object *target;

  switch (o->state)
  {
    case 0:
    {
      // find the Toroko object we are to home in on
      target = mbubble_find_target();
      if (!target)
      {
        o->state = 9999;
        return;
      }

      o->xmark = target->x - (6 * CSFI);
      o->ymark = target->y - (6 * CSFI);

      ThrowObject(o, o->xmark, o->ymark, 0, (2 * CSFI));
      o->state = 1;

      // correct values: 0x3F0, 0xAE
      stat("Computed toss values xi: 0x%x, 0x%x", o->xinertia, o->yinertia);
      stat("Target x/y: 0x%x, 0x%x", target->x, target->y);
    }
    case 1:
      ANIMATE(1, 0, 1);

      if (abs(o->x - o->xmark) <= (3 * CSFI) && abs(o->y - o->ymark) <= (3 * CSFI))
      {
        o->state = 2;
        o->frame = 2;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BUBBLE);

        if ((target = mbubble_find_target()))
        {
          target->Delete();
          //					target->invisible = true;
        }
      }
      break;

    case 2:
    {
      ANIMATE(1, 2, 3);

      o->xinertia -= 0x20;
      o->yinertia -= 0x20;
      LIMITX(0x5FF);
      LIMITY(0x5FF);

      if (o->y < -1000)
        o->Delete();
    }
    break;
  }
}

static Object *mbubble_find_target(void)
{
  Object *target = FindObjectByID2(1000);
  if (!target)
  {
    staterr("ai_miserys_bubble: failed to find a target object with ID2=1000");
    return NULL;
  }

  return target;
}

/*
void c------------------------------() {}
*/

void ai_misery_stand(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state = 1;
    case 1:
      o->frame = 2;
      randblink(o, 3, 4);
      break;

    case 20: // she flys away
      o->state    = 21;
      o->frame    = 0;
      o->yinertia = 0;
      o->flags |= FLAG_IGNORE_SOLID;
    case 21:
      o->yinertia -= 0x20;
      if (o->y < -0x1000)
        o->Delete();
      break;

    // big spell
    // she flashes, then a clap of thunder,
    // and she teleports away.
    case 25:
      o->state     = 26;
      o->timer     = 0;
      o->frame     = 5;
      o->animtimer = 0;
    case 26:
      ANIMATE(0, 5, 7);
      if (++o->timer == 20)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LIGHTNING_STRIKE);
        flashscreen.Start();
        o->state = 27;
        o->timer = 0;
        o->frame = 4;
      }
      break;
    case 27:
    {
      if (++o->timer > 50)
      { // return to standing
        o->state = 0;
      }
    }
    break;

    case 30: // she throws up her staff like she's summoning something
      o->timer = 0;
      o->state++;
      o->frame = 2;
    case 31:
      if (o->timer == 10)
        o->frame = 4;
      if (o->timer == 130)
        o->state = 1;
      o->timer++;
      break;

    // fire at DOCTOR_GHOST
    case 40:
    {
      o->state = 41;
      o->timer = 0;
      o->frame = 4;
    }
    case 41:
    {
      o->timer++;

      if (o->timer == 30 || o->timer == 40 || o->timer == 50)
      {
        Object *shot   = CreateObject(o->x + (16 * CSFI), o->y, OBJ_IGOR_SHOT);
        shot->xinertia = 0x600;
        shot->yinertia = random(-0x200, 0);

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SNAKE_FIRE);
      }

      if (o->timer > 50)
        o->state = 0;
    }
    break;
  }
}
