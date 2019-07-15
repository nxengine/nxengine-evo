#include "ma_pignon.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Tileset.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../ai.h"
#include "../stdai.h"

enum
{
  MP_Fight_Start  = 100, // scripted
  MP_BaseState    = 110,
  MP_Jump         = 120,
  MP_In_Air       = 130,
  MP_Landed       = 140,
  MP_ChargeAttack = 200,
  MP_Hit_Wall     = 220,
  MP_CloneAttack  = 300,
  MP_Fly_Up       = 310,
  MP_Spawn_Clones = 330,
  MP_Defeated     = 500 // scripted
};

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_MA_PIGNON, ai_ma_pignon);

  ONTICK(OBJ_MA_PIGNON_ROCK, ai_ma_pignon_rock);
  ONTICK(OBJ_MA_PIGNON_CLONE, ai_ma_pignon_clone);
}

/*
void c------------------------------() {}
*/

void ai_ma_pignon(Object *o)
{
  /*debug("state: %d", o->state);
  debug("timer: %d", o->timer);
  debug("timer2: %d", o->timer2);
  debug("timer3: %d", o->timer3);
  debug("xinertia: %d", o->xinertia);
  debug("yinertia: %d", o->yinertia);
  debug("frame: %d", o->frame);
  */

  switch (o->state)
  {
    case 0:
    {
      o->SnapToGround();
      o->state = 1;
    }
    case 1:
    {
      FACEPLAYERIFNEARBY;
      o->frame = 0;
      randblink(o);
    }
    break;

    case MP_Fight_Start: // set by script
    {
      o->state  = MP_BaseState;
      o->timer  = 0;
      o->timer2 = 0;
      o->flags |= FLAG_SHOOTABLE;
    }
    case MP_BaseState:
    {
      FACEPLAYER;
      o->damage = 1;
      o->frame  = 0;

      if (++o->timer > 4)
      {
        o->timer = 0;
        o->state = MP_Jump;

        if (++o->timer3 > 12)
        {
          o->timer3 = 0;
          o->state  = MP_CloneAttack;
        }
      }
    }
    break;

    case MP_Jump: // pause a moment and jump
    {
      o->frame = 2;
      if (++o->timer > 4)
      {
        o->state = MP_In_Air;
        o->frame = 3;

        o->xinertia = random(-0x400, 0x400);
        o->yinertia = -0x800;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);
        o->timer2++;
      }
    }
    break;

    case MP_In_Air: // jumping or falling after clone attack
    {
      o->yinertia += 0x80;

      // for when falling back onscreen after clone attack
      if (o->y > (8 * TILE_H) * CSFI)
        o->flags &= ~FLAG_IGNORE_SOLID;
      else
        o->blockd = false;

      // bounce off walls
      if ((o->blockl && o->xinertia < 0) || (o->blockr && o->xinertia > 0))
      {
        o->xinertia = -o->xinertia;
      }

      FACEPLAYER;

      // select frame
      if (o->yinertia < -0x200)
      {
        o->frame = 3;
      }
      else if (o->yinertia > 0x200)
      {
        o->frame = 4;
      }
      else
      {
        o->frame = 0;
      }

      if (o->blockd && o->yinertia > 0)
      {
        o->state    = MP_Landed;
        o->timer    = 0;
        o->frame    = 2;
        o->xinertia = 0;
      }

      if (o->timer2 > 4)
      {
        if (player->y < (o->y + 0x800))
        {
          o->state    = MP_ChargeAttack;
          o->timer    = 0;
          o->xinertia = 0;
          o->yinertia = 0;
        }
      }
    }
    break;

    case MP_Landed:
    {
      o->frame = 2;
      if (++o->timer > 4)
      {
        o->state = MP_BaseState;
      }
    }
    break;

    case MP_ChargeAttack: // charge attack
    {
      o->frame = 5;
      if (++o->timer > 10)
      {
        o->state = MP_ChargeAttack + 1;
        o->frame = 6;

        XMOVE(0x5ff);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);

        o->flags &= ~FLAG_SHOOTABLE;
        o->flags |= FLAG_INVULNERABLE;
        o->damage = 10;
      }
    }
    break;
    case MP_ChargeAttack + 1: // in-air during charge attack
    {
      ANIMATE(0, 6, 7);

      if ((o->xinertia < 0 && o->blockl) || (o->xinertia > 0 && o->blockr))
      {
        o->state = MP_Hit_Wall;
      }
    }
    break;

    case MP_Hit_Wall: // hit wall
    {
      o->state++;
      o->timer = 0;
      quake(16);
    }
    case MP_Hit_Wall + 1:
    {
      o->damage = 4;
      ANIMATE(0, 6, 7);

      if ((++o->timer % 6) == 0)
      {
        int x = (random(4, 16) * TILE_W) * CSFI;
        CreateObject(x, (16 * CSFI), OBJ_MA_PIGNON_ROCK);
      }

      if (o->timer > 30)
      {
        o->timer2 = 0;
        o->state  = MP_In_Air;

        o->flags |= FLAG_SHOOTABLE;
        o->flags &= ~FLAG_INVULNERABLE;

        o->damage = 3;
      }
    }
    break;

    case MP_CloneAttack: // begin clone-attack sequence
    {
      o->state++;
      o->frame = 9;
      FACEPLAYER;
    }
    case MP_CloneAttack + 1: // walk at player before attack
    {
      ANIMATE(0, 9, 11);

      XMOVE(0x400);
      if (pdistlx(3 * CSFI))
      {
        o->state    = MP_Fly_Up;
        o->timer    = 0;
        o->frame    = 2;
        o->xinertia = 0;
      }
    }
    break;

    case MP_Fly_Up: // jump and fly up for clone attack
    {
      o->frame = 2;
      if (++o->timer > 4)
      {
        o->state++;
        o->frame    = 12;
        o->yinertia = -0x800;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);

        o->flags |= FLAG_IGNORE_SOLID;
        o->flags &= ~FLAG_SHOOTABLE;
        o->flags |= FLAG_INVULNERABLE;

        o->damage = 10;
      }
    }
    break;
    case MP_Fly_Up + 1: // flying up
    {
      ANIMATE(0, 12, 13);

      if (o->y < (16 * CSFI))
        o->state = MP_Spawn_Clones;
    }
    break;

    case MP_Spawn_Clones: // offscreen, spawning clones
    {
      o->yinertia = 0;
      o->state++;
      o->timer = 0;

      quake(10);
    }
    case MP_Spawn_Clones + 1:
    {
      ANIMATE(0, 12, 13);

      if ((++o->timer % 6) == 0)
      {
        int x = (random(4, 16) * TILE_W) * CSFI;
        CreateObject(x, (16 * CSFI), OBJ_MA_PIGNON_CLONE);
      }

      if (o->timer > 30)
      {
        o->timer2 = 0;
        o->state  = MP_In_Air; // fall back down to ground

        o->flags |= FLAG_SHOOTABLE;
        o->flags &= ~FLAG_INVULNERABLE;
      }
    }
    break;

    case MP_Defeated: // defeated -- set by script
    {
      KillObjectsOfType(OBJ_MA_PIGNON_CLONE);
      o->flags &= ~FLAG_SHOOTABLE;
      o->state++;
      o->timer  = 0;
      o->frame  = 8;
      o->damage = 0;
    }
    case MP_Defeated + 1:
    {
      o->yinertia += 0x20;
      if (o->blockd)
      {
        o->xinertia *= 7;
        o->xinertia /= 8;
      }

      o->display_xoff = (++o->timer & 1);
    }
    break;
  }

  // ma pignon is invulnerable to missiles and Blade.
  if (o->state >= MP_Fight_Start && o->state < MP_Defeated)
  {
    // ....he's invulnerable anyway during these two states so don't mess with that.
    if (o->state != MP_ChargeAttack + 1 && o->state != MP_Fly_Up + 1)
    {
      bool found_weapons = false;
      if (o->type != OBJ_MA_PIGNON_CLONE)
      {
        Object *c;
        for(int i =0; i < 64; i++)
        {
          if (bullets[i] != NULL)
          {
            c = bullets[i];
            if (c->type == OBJ_MISSILE_SHOT || c->type == OBJ_SUPERMISSILE_SHOT || c->type == OBJ_MISSILE_BOOM_SPAWNER
                || c->type == OBJ_BLADE12_SHOT || c->type == OBJ_BLADE3_SHOT || c->type == OBJ_BLADE_SLASH)
            {
              found_weapons = true;
              break;
            }
          }
        }
      }

      if (found_weapons)
      {
        o->flags &= ~FLAG_SHOOTABLE;
        o->flags |= FLAG_INVULNERABLE;
      }
      else
      {
        o->flags |= FLAG_SHOOTABLE;
        o->flags &= ~FLAG_INVULNERABLE;
      }
    }
  }

  // don't use LIMITY, it limits upwards too and breaks his jumps
  if (o->yinertia > 0x5ff)
    o->yinertia = 0x5ff;
}

/*
void c------------------------------() {}
*/

void ai_ma_pignon_rock(Object *o)
{
  ANIMATE(6, 0, 2);

  switch (o->state)
  {
    case 0:
    {
      o->timer3 = 0;
      o->state  = 1;
      o->flags |= (FLAG_INVULNERABLE | FLAG_IGNORE_SOLID);
      o->frame  = random(0, 2);
      o->damage = 10;
    }
    case 1:
    {
      o->yinertia += 0x40;
      LIMITY(0x700);

      if (o->y > (8 * TILE_H) * CSFI)
      {
        o->flags &= ~FLAG_IGNORE_SOLID;

        if (o->blockd)
        {
          o->yinertia = -0x200;
          o->state    = 2;
          o->flags |= FLAG_IGNORE_SOLID;

          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
          game.quaketime = 10;

          // these smoke clouds appear BEHIND the map tiles
          for (int i = 0; i < 2; i++)
          {
            Object *smoke   = CreateObject(o->CenterX() + random(-12, 12) * CSFI, o->Bottom() + (16 * CSFI),
                                         OBJ_SMOKE_CLOUD);
            smoke->xinertia = random(-0x155, 0x155);
            smoke->yinertia = random(-0x600, 0);
          }
        }
      }
    }
    break;

    case 2:
    {
      o->yinertia += 0x40;
      if (o->y > (map.ysize * TILE_H) * CSFI)
        o->Delete();
    }
    break;
  }
}

void ai_ma_pignon_clone(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->frame = 3;
      o->yinertia += 0x80;
      LIMITY(0x5ff);

      if (o->y > (8 * TILE_H) * CSFI)
      {
        o->state = 130;
        o->flags &= ~FLAG_IGNORE_SOLID;
      }
    }
    break;

    default:
      o->timer2 = o->timer3 = 0;
      ai_ma_pignon(o);
      break;
  }

  if (++o->substate > 300)
  {
    effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
    o->Delete();
  }
}
