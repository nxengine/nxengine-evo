#include "gaudi.h"

#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Renderer.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"
using namespace NXE::Graphics;
#include "../../autogen/sprites.h"
#include "../../graphics/sprites.h"

#define GAUDI_HP 15
#define GAUDI_FLYING_HP 15
#define GAUDI_ARMORED_HP 15

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_GAUDI, ai_gaudi);

  ONTICK(OBJ_GAUDI_ARMORED, ai_gaudi_armored);
  ONTICK(OBJ_GAUDI_ARMORED_SHOT, ai_gaudi_armored_shot);

  ONTICK(OBJ_GAUDI_FLYING, ai_gaudi_flying);
  ONTICK(OBJ_GAUDI_FLYING_SHOT, ai_generic_angled_shot);

  ONTICK(OBJ_GAUDI_DYING, ai_gaudi_dying);
}

/*
void c------------------------------() {}
*/

void ai_gaudi(Object *o)
{
  if (o->hp <= (1000 - GAUDI_HP))
  {
    o->ChangeType(OBJ_GAUDI_DYING);
    ai_gaudi_dying(o);
    return;
  }

  if (!(pdistlx(Renderer::getInstance()->screenWidth * CSFI) && pdistly(Renderer::getInstance()->screenHeight * CSFI)))
    return;

  switch (o->state)
  {
    case 0:
    {
      // Gaudi's in shop
      if (o->flags & FLAG_SCRIPTONACTIVATE)
      {
        o->damage = 0;
        o->flags &= ~FLAG_SHOOTABLE;
      }

      o->xinertia = 0;
      o->state    = 1;
    }
    case 1:
    {
      o->frame = 0;
      randblink(o, 1, 20, 100);

      if (!random(0, 100))
      {
        if (random(0, 1))
        {
          o->dir ^= 1;
        }
        else
        {
          o->state = 10;
        }
      }
    }
    break;

    case 10: // walking
    {
      o->state = 11;
      o->timer = random(25, 100); // how long to walk for

      o->frame     = 2;
      o->animtimer = 0;
    }
    case 11:
    {
      ANIMATE(3, 2, 5);
      XMOVE(0x200);

      // time to stop walking?
      if (--o->timer <= 0)
        o->state = 0;

      // try to jump over any walls we come to
      if ((o->xinertia < 0 && o->blockl) || (o->xinertia > 0 && o->blockr))
      {
        o->yinertia = -0x5ff;
        o->frame    = 2;
        o->state    = 20;

        if (!player->inputs_locked) // no sound during ending cutscene
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);
      }
    }
    break;

    case 20: // jumping
    {
      // landed?
      if (o->yinertia >= 0 && o->blockd)
      {
        o->xinertia = 0;
        o->state    = 21;
        o->frame    = 12;
        o->timer    = 0;

        if (!player->inputs_locked) // no sound during ending cutscene
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
      }

      // count how long we've been touching the wall
      // we're trying to jump over..if it's not working
      // go the other way.
      if ((o->dir == LEFT && o->blockl) || (o->dir == RIGHT && o->blockr))
      {
        if (++o->timer2 > 10)
        {
          o->timer2 = 0;
          o->dir ^= 1;
        }
      }
      else
      {
        o->timer2 = 0;
      }

      XMOVE(0x100);
    }
    break;

    case 21: // landed from jump
    {
      if (++o->timer > 10)
        o->state = 0;
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_gaudi_dying(Object *o)
{
  switch (o->state)
  {
    case 0: // just died (initilizing)
    {
      o->flags &= ~(FLAG_SHOOTABLE | FLAG_IGNORE_SOLID | FLAG_SHOW_FLOATTEXT);
      o->damage = 0;

      o->sprite = SPR_GAUDI;
      o->frame  = 9;

      o->yinertia = -0x200;
      XMOVE(-0x100);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_HURT_SMALL);

      o->state = 1;
    }
    break;

    case 1: // flying backwards through air
    {
      if (o->blockd && o->yinertia >= 0)
      {
        o->frame = 10;
        o->state = 2;
        o->timer = 0;
      }
    }
    break;

    case 2: // landed, shake
    {
      o->xinertia *= 8;
      o->xinertia /= 9;
      ANIMATE(3, 10, 11);

      if (++o->timer > 50)
      { // this deletes object while generating smoke effects and boom
        o->DealDamage(10000);
      }
    }
    break;
  }

  o->yinertia += 0x20;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_gaudi_flying(Object *o)
{
  if (o->hp <= (1000 - GAUDI_FLYING_HP))
  {
    if (o->dir == LEFT)
      o->x -= (2 * CSFI);
    else
      o->x += (2 * CSFI);

    o->ChangeType(OBJ_GAUDI_DYING);
    ai_gaudi_dying(o);
    return;
  }

  if (!(pdistlx(Renderer::getInstance()->screenWidth * CSFI) && pdistly(Renderer::getInstance()->screenHeight * CSFI)))
    return;

  switch (o->state)
  {
    case 0:
    {
      vector_from_angle(random(0, 255), (1 * CSFI), &o->xinertia, &o->yinertia);
      o->xmark = o->x + (o->xinertia * 8);
      o->ymark = o->y + (o->yinertia * 8);

      o->timer2 = 120;
    }
    case 1:
    {
      o->timer = random(70, 150);
      o->state = 2;
    }
    case 2:
    {
      if (!--o->timer)
      {
        o->state = 3;
        o->frame |= 0x02; // switch us into using flashing purple animation
      }
    }
    break;

    case 3: // preparing to fire
    {
      o->timer++;

      if (++o->timer > 30)
      {
        EmFireAngledShot(o, OBJ_GAUDI_FLYING_SHOT, 6, 0x500);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);

        o->state = 1;
        o->frame &= 1; // stop flashing purple
      }
    }
  }

  o->frame ^= 1; // animate wings
  FACEPLAYER;

  // sinusoidal circling pattern
  o->xinertia += (o->x > o->xmark) ? -0x10 : 0x10;
  o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;
  LIMITX(0x200);
  LIMITY(0x200);
}

/*
void c------------------------------() {}
*/

void ai_gaudi_armored(Object *o)
{
  if (o->hp <= (1000 - GAUDI_ARMORED_HP))
  {
    o->ChangeType(OBJ_GAUDI_DYING);
    ai_gaudi_dying(o);
    return;
  }

  if (!(pdistlx(Renderer::getInstance()->screenWidth * CSFI) && pdistly(Renderer::getInstance()->screenHeight * CSFI)))
    return;

  FACEPLAYER;

  // debug("%d", o->state);
  // debugVline(o->x, 0, 0, 255);
  // debugVline(o->xmark, 0, 255, 0);

  switch (o->state)
  {
    case 0:
    {
      o->xmark = o->x;
      o->state = 1;
    }
    case 1:
    {
      o->frame    = 0;
      o->xinertia = 0;

      if (++o->timer >= 5)
      {
        if (pdistlx(192 * CSFI) && pdistly(160 * CSFI))
        { // begin hopping
          o->state = 10;
          o->timer = 0;
          o->frame = 1;
        }
      }
    }
    break;

    case 10: // on ground inbetween hops
    {
      if (++o->timer > 3)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);
        o->frame = 2;
        o->timer = 0;

        if (++o->timer2 < 3)
        { // hopping back and forth
          o->state    = 20;
          o->yinertia = -0x200;
          o->xinertia = (o->x < o->xmark) ? 0x200 : -0x200;
        }
        else
        { // big jump and attack
          o->state    = 30;
          o->yinertia = -0x600;
          o->xinertia = (o->x < o->xmark) ? 0x80 : -0x80;

          o->timer2 = 0;
        }
      }
    }
    break;

    case 20: // jumping (small hop)
    {
      // landed?
      if (o->blockd && o->yinertia >= 0)
      {
        // drop sub-pixel precision
        // (required to maintain stability of back-and-forth sequence).
        o->y /= CSFI;
        o->y *= CSFI;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
        o->state = 40;
        o->frame = 1;
        o->timer = 0;
      }
    }
    break;

    case 30: // jumping (big jump + attack)
    {
      o->timer++;

      // throw attacks at player
      if (o->timer == 30 || o->timer == 40)
      {
        EmFireAngledShot(o, OBJ_GAUDI_ARMORED_SHOT, 6, 0x600);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);

        o->frame = 3;
        o->CurlyTargetHere();
      }

      // stop throwing animation
      if (o->timer == 35 || o->timer == 45)
        o->frame = 2;

      if (o->blockd && o->yinertia > 0)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
        o->state = 40;
        o->frame = 1;
        o->timer = 0;
      }
    }
    break;

    case 40: // landed
    {
      o->xinertia *= 7;
      o->xinertia /= 8;

      if (++o->timer >= 2)
      {
        // stat("dtt= %d", abs(o->xmark - o->x) / CSFI);
        o->frame    = 0;
        o->xinertia = 0;

        o->state = 1;
        o->timer = 0;
      }
    }
    break;
  }

  o->yinertia += 0x33;
  LIMITY(0x5ff);
}

void ai_gaudi_armored_shot(Object *o)
{
  ANIMATE(0, 0, 2);

  switch (o->state)
  {
    case 0:
    {
      bool bounced = false;
      if (o->blockl && o->xinertia <= 0)
      {
        o->xinertia = 0x200;
        bounced     = true;
      }
      if (o->blockr && o->xinertia >= 0)
      {
        o->xinertia = -0x200;
        bounced     = true;
      }
      if (o->blockd && o->yinertia >= 0)
      {
        o->yinertia = -0x200;
        bounced     = true;
      }
      if (o->blocku && o->yinertia <= 0)
      {
        o->yinertia = 0x200;
        bounced     = true;
      }

      if (bounced)
      {
        o->state = 1;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TINK);
      }
    }
    break;

    case 1:
    {
      o->yinertia += 0x40;
      LIMITY(0x5ff);

      if (o->blockd && o->yinertia >= 0)
      {
        effect(o->CenterX(), o->CenterY(), EFFECT_STARPOOF);
        o->Delete();
        return;
      }
    }
    break;
  }
}
