#include "igor.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

#define IGOR_DEFEAT_FRAME 12

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_NPC_IGOR, ai_npc_igor);

  ONTICK(OBJ_BOSS_IGOR, ai_boss_igor);
  ONTICK(OBJ_IGOR_SHOT, ai_generic_angled_shot);

  ONTICK(OBJ_BOSS_IGOR_DEFEATED, ai_boss_igor_defeated);
}

/*
void c------------------------------() {}
*/

// cutscene igor
void ai_npc_igor(Object *o)
{
  switch (o->state)
  {
    case 0: // init, standing/panting
      o->xinertia  = 0;
      o->frame     = 0;
      o->animtimer = 0;
      o->state     = 1;
    case 1:
      ANIMATE(5, 0, 1);
      break;

    case 2: // walking
      o->state     = 3;
      o->frame     = 2;
      o->animtimer = 0;
    case 3:
      ANIMATE(3, 2, 5);
      XMOVE(0x200);
      break;

    case 4: // punch
      o->xinertia = 0;
      o->state    = 5;
      o->timer    = 0;
    case 5:
      o->frame = 6;
      if (++o->timer > 10)
      {
        o->timer = 0;
        o->state = 6;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPL_SMALL);
      }
      break;
    case 6:
      o->frame = 7;
      if (++o->timer > 8)
      {
        o->state = 0;
        o->frame = 0;
      }
      break;

    case 7:
      o->state = 1;
      break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5FF);
}

/*
void c------------------------------() {}
*/

// boss-fight igor
void ai_boss_igor(Object *o)
{
  enum
  {
    STATE_INIT = 0,
    STATE_STAND,

    STATE_BEGIN_ATTACK,
    STATE_WALK,

    STATE_JUMPING,
    STATE_LANDED,

    STATE_PUNCH,
    STATE_PUNCH_2,
    STATE_PUNCH_3,

    STATE_MOUTH_BLAST,
    STATE_MOUTH_BLAST_2
  };

  switch (o->state)
  {
    case STATE_INIT:
    {
      o->damage   = 0;
      o->xinertia = 0;

      o->state     = STATE_STAND;
      o->frame     = 0;
      o->animtimer = 0;
    }
    case STATE_STAND:
    {
      ANIMATE(5, 0, 1);
      if (++o->timer > 50)
      {
        o->state = STATE_BEGIN_ATTACK;
      }
    }
    break;

    case STATE_BEGIN_ATTACK:
    {
      o->state     = STATE_WALK;
      o->frame     = 2;
      o->animtimer = 0;
      o->timer     = 0;

      FACEPLAYER;
      o->igor.fireattack = false;

      // when health is less than halfway, then use
      // the mouth blast attack every third time.
      if (++o->timer2 >= 3 && o->hp <= (objprop[o->type].initial_hp / 2))
      {
        o->timer2          = 0;
        o->igor.fireattack = true;
        o->dir ^= 1; // walk away from player
      }

    } // fall thru
    case STATE_WALK:
    {
      ANIMATE(3, 2, 5);
      XMOVE(0x200);

      if (o->igor.fireattack)
      { // begin mouth-blast attack
        if (++o->timer > 16)
        {
          o->state    = STATE_MOUTH_BLAST;
          o->xinertia = 0;
          o->frame    = 10;
        }
      }
      else
      {
        if (o->dir == LEFT)
        {
          if (o->x <= player->x + player->Width())
            o->state = STATE_PUNCH;
        }
        else
        {
          if (o->x + o->Width() >= player->x)
            o->state = STATE_PUNCH;
        }

        // if we don't reach him after a while, do a jump
        if (++o->timer > 50)
        {
          o->frame    = 10;
          o->yinertia = -0x400;
          o->state    = STATE_JUMPING;
          o->timer    = 0;

          o->xinertia = 3 * o->xinertia / 2;

          o->damage = 2;
        }
      }
    }
    break;

    case STATE_PUNCH:
    {
      o->xinertia = 0;

      o->state = STATE_PUNCH_2;
      o->frame = 6;
      o->timer = 0;
    }
    case STATE_PUNCH_2:
    {
      if (++o->timer > 12)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EXPL_SMALL);

        // sprite appears identical, but has a wider bounding box.
        o->sprite = SPR_IGOR_PUNCHING;
        o->damage = 5;

        o->state = STATE_PUNCH_3;
        o->frame = 7;
        o->timer = 0;
      }
    }
    break;
    case STATE_PUNCH_3:
    {
      if (++o->timer > 10)
      {
        o->state  = STATE_INIT;
        o->frame  = 0;
        o->damage = 0;

        // return to normal-size bounding box
        o->sprite = SPR_IGOR;
      }
    }
    break;

    case STATE_JUMPING:
    {
      if (o->blockd)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_QUAKE);
        SmokeSide(o, 4, DOWN);

        o->state = STATE_LANDED;
        o->frame = 11;
        o->timer = 0;
      }
    }
    break;

    case STATE_LANDED:
    {
      o->xinertia = 0;
      if (++o->timer > 10)
      {
        o->state  = STATE_INIT;
        o->frame  = 0;
        o->damage = 0;
      }
    }
    break;

    case STATE_MOUTH_BLAST:
    {
      FACEPLAYER;
      o->timer = 0;
      o->state++;
    }
    case STATE_MOUTH_BLAST_2:
    {
      o->timer++;

      // flash mouth
      o->frame = 8;
      if (o->timer > 50 && (o->timer & 2))
        o->frame = 9;

      // fire shots
      if (o->timer > 100)
      {
        if ((o->timer % 6) == 1)
        {
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
          Object *shot = SpawnObjectAtActionPoint(o, OBJ_IGOR_SHOT);

          int angle = (o->dir == LEFT) ? 136 : 248;
          angle += random(-16, 16);
          ThrowObjectAtAngle(shot, angle, 0x580);
        }

        if (o->timer > 132) // fires 6 shots
        {
          o->state = STATE_INIT;
          o->timer = 0;
        }
      }
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_boss_igor_defeated(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->xinertia = 0;
      o->yinertia = 0;
      o->flags &= ~FLAG_SHOOTABLE;
      o->frame = IGOR_DEFEAT_FRAME;

      FACEPLAYER;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BIG_CRASH);

      SmokeBoomUp(o);

      o->timer = 0;
      o->state = 1;
    }
    break;

    case 1:
    {
      if ((++o->timer % 5) == 0)
        smoke_puff(o, false);

      o->display_xoff = 0;
      if (o->timer & 2)
        o->display_xoff = -1;

      if (o->timer > 100)
      {
        o->xmark = o->x;
        o->ymark = o->y;

        o->timer = 0;
        o->state = 2;
      }
    }
    break;

    case 2:
    {
      if ((++o->timer % 9) == 0)
        smoke_puff(o, false);

      // alternate between big and small sprites
      // (frenzied/not-frenzied forms)
      if (o->timer & 2)
      {
        o->sprite = SPR_IGOR_DEFEATED;
        o->frame  = 0;

        if (o->dir == LEFT)
          o->x = o->xmark + (12 * CSFI);
        else
          o->x = o->xmark + (2 * CSFI);

        o->y = o->ymark + (12 * CSFI);
      }
      else
      {
        o->sprite = SPR_IGOR;
        o->frame  = IGOR_DEFEAT_FRAME;

        o->x = o->xmark;
        o->y = o->ymark;
      }

      if (o->timer > 150)
      {
        o->state = 3;
        o->frame = 0;
        o->timer = 0;
      }
    }
    break;

    case 3:
    {
      if (++o->timer > 50)
      {
        o->timer = 0;
        o->frame++;

        if (o->frame >= 2)
          o->state = 4;
      }

      if (o->frame == 0 && (o->timer % 16) == 0)
        smoke_puff(o, false);
    }
    break;

    case 4:
      break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

static void smoke_puff(Object *o, bool initial)
{
  Object *smoke;

  smoke = CreateObject(o->CenterX() + random(-16, 16) * CSFI, o->CenterY() + random(-16, 16) * CSFI,
                       OBJ_SMOKE_CLOUD);

  if (initial)
  {
    smoke->xinertia = random(-0x155, 0x155);
    smoke->yinertia = random(-0x600, 0);
  }
  else
  {
    smoke->xinertia = random(-0x600, 0x600);
    smoke->yinertia = random(-0x600, 0x600);
  }
}
