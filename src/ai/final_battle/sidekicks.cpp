#include "sidekicks.h"

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
#include "../stdai.h"

using namespace NXE::Graphics;

#define SUE_BASE 20
#define SUE_PREPARE_ATTACK 30
#define SUE_SOMERSAULT 40
#define SUE_DASH 50
#define SUE_SOMERSAULT_HIT 60

// both sue and misery
#define SIDEKICK_CORE_DEFEATED 99 // core defeated (script-triggered)
#define SIDEKICK_DEFEATED 100     // sidekick defeated
#define SIDEKICK_CORE_DEFEATED_2 110

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_SUE_FRENZIED, ai_sue_frenzied);
  ONTICK(OBJ_MISERY_FRENZIED, ai_misery_frenzied);

  ONTICK(OBJ_MISERY_CRITTER, ai_misery_critter);
  ONTICK(OBJ_MISERY_BAT, ai_misery_bat);
  ONTICK(OBJ_MISERY_MISSILE, ai_misery_missile);
}

bool sue_being_hurt;
bool sue_was_killed;

/*
void c------------------------------() {}
*/

void ai_misery_frenzied(Object *o)
{
  // AIDEBUG;
  sidekick_run_defeated(o, 600);

  switch (o->state)
  {
    case 0:
    {
      o->state       = 1;
      sue_being_hurt = sue_was_killed = false;

      o->savedhp = o->hp;
      o->nxflags |= NXFLAG_SLOW_X_WHEN_HURT;

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
      o->timer = 1;
    }
    case 1: // transforming
    {
      o->timer++;

      if (o->timer == 2)
      { // frenzied
        o->sprite = SPR_MISERY_FRENZIED;
        o->frame  = 9;
        o->x -= 0x1000;
        o->y -= 0x2000;
      }

      if (o->timer == 4)
      { // normal
        o->timer = 0;

        o->sprite = SPR_MISERY;
        o->frame  = 2;
        o->x += 0x1000;
        o->y += 0x2000;
      }

      if (++o->timer2 >= 50)
      {
        o->timer2 = 0;
        o->state  = 2;
      }
    }
    break;

    case 10: // hold at "being transformed" frame
    {
      o->state = 11;
      o->frame = 9;
    }
    break;

    case 20: // fight begin / base state
    {
      o->state     = 21;
      o->timer     = 0;
      o->frame     = 0;
      o->animtimer = 0;
    }
    case 21:
    {
      o->xinertia *= 7;
      o->xinertia /= 8;
      o->yinertia *= 7;
      o->yinertia /= 8;

      ANIMATE(20, 0, 1);

      if (++o->timer > 100)
        o->state = 30;

      FACEPLAYER;
    }
    break;

    case 30:
    {
      o->state   = 31;
      o->timer   = 0;
      o->frame   = 2;
      o->savedhp = o->hp;
    }
    case 31:
    {
      ANIMATE(1, 2, 3);

      if (o->blockd)
        o->yinertia = -0x200;

      int core_x = game.stageboss.object ? game.stageboss.object->x : 0;

      o->xinertia += (o->x > core_x) ? -0x20 : 0x20;
      o->yinertia += (o->y > player->y) ? -0x10 : 0x10;
      LIMITX(0x200);
      LIMITY(0x200);

      FACEPLAYER;

      if (++o->timer > 150)
      {
        // she attacks with normal critters if you attack either her or Sue.
        if ((o->savedhp - o->hp) > 20 || sue_being_hurt)
        {
          sue_being_hurt = false;
          o->state       = 40;
        }
      }

      // she attacks periodically with fishy missiles if you killed Sue.
      if (o->timer > 250 && sue_was_killed)
        o->state = 50;
    }
    break;

    case 40: // spawn bats/critters
    {
      o->state    = 41;
      o->timer    = 0;
      o->xinertia = 0;
      o->yinertia = 0;
      FACEPLAYER;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CHARGE_GUN);

      // if you are below the 2nd little platform on the left,
      // she spawns critters, else bats.
      o->timer3 = (player->y >= MAPY(10)) ? OBJ_MISERY_CRITTER : OBJ_MISERY_BAT;
    }
    case 41:
    {
      o->timer++;
      o->frame = (o->timer & 2) ? 4 : 5;

      if ((o->timer % 6) == 1)
      {
        int x, y;

        if (o->timer3 == OBJ_MISERY_CRITTER)
        {
          x = o->x + (random(-64, 64) * CSFI);
          y = o->y + (random(-32, 32) * CSFI);
        }
        else
        {
          x = o->x + (random(-32, 32) * CSFI);
          y = o->y + (random(-64, 64) * CSFI);
        }

        if (x < MAPX(2))
          x = MAPX(2);
        if (x > MAPX(map.xsize - 3))
          x = MAPX(map.xsize - 3);

        if (y < MAPY(2))
          y = MAPY(2);
        if (y > MAPY(map.ysize - 3))
          y = MAPY(map.ysize - 3);

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
        CreateObject(x, y, o->timer3)->invisible = true;
      }

      if (o->timer > 50)
      {
        o->state = 42;
        o->timer = 0;
        FACEPLAYER;
      }
    }
    break;

    case 42:
    {
      o->frame = 6;

      if (++o->timer > 50)
      {
        o->yinertia = -0x200;
        XMOVE(-0x200);

        o->state = 30;
      }
    }
    break;

    case 50: // spawn fishy missiles
    {
      o->state    = 51;
      o->timer    = 0;
      o->xinertia = 0;
      o->yinertia = 0;
      FACEPLAYER;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CHARGE_GUN);
    }
    case 51:
    {
      o->timer++;
      o->frame = (o->timer & 2) ? 4 : 5;

      int rate = (player->equipmask & EQUIP_BOOSTER20) ? 10 : 24;

      if ((o->timer % rate) == 1)
      {
        // pattern: booster=[0,1,3,1,2,0], no-booster=[0,0,0]:
        int angindex = (o->timer / 6) & 3;
        fm_spawn_missile(o, angindex);
      }

      if (++o->timer > 50)
      {
        o->state = 42;
        o->timer = 0;
        FACEPLAYER;
      }
    }
    break;
  }
}

// spawn a fishy missile in the given direction
static Object *fm_spawn_missile(Object *o, int angindex)
{
  static const int ang_table_left[]  = {0xD8, 0xEC, 0x14, 0x28};
  static const int ang_table_right[] = {0x58, 0x6C, 0x94, 0xA8};

  Object *shot = CreateObject(o->x, o->y, OBJ_MISERY_MISSILE);
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);

  if (o->dir == LEFT)
  {
    shot->x += (10 * CSFI);
    shot->angle = ang_table_left[angindex];
  }
  else
  {
    shot->x -= (10 * CSFI);
    shot->angle = ang_table_right[angindex];
  }

  return shot;
}

/*
void c------------------------------() {}
*/

void ai_misery_critter(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (++o->timer > 16)
      {
        o->frame     = 2;
        o->invisible = false;
        FACEPLAYER;

        o->state  = 10;
        o->damage = 2;
        o->flags |= FLAG_SHOOTABLE;
      }
    }
    break;

    case 10:
    {
      if (o->blockd && o->yinertia >= 0)
      {
        o->state    = 11;
        o->frame    = 0;
        o->timer    = 0;
        o->xinertia = 0;

        FACEPLAYER;
      }
    }
    break;

    case 11:
    {
      if (++o->timer > 10)
      {
        if (++o->timer2 > 4)
          o->state = 12;
        else
          o->state = 10;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);

        o->yinertia = -0x600;
        XMOVE(0x200);

        o->frame = 2;
      }
    }
    break;

    case 12:
    {
      o->flags |= FLAG_IGNORE_SOLID;
      if (o->y > MAPY(map.ysize))
      {
        o->Delete();
      }
    }
    break;
  }

  if (o->state >= 10)
  {
    o->yinertia += 0x40;
    if (o->yinertia > 0x5ff)
      o->yinertia = 0x5ff;
  }
}

void ai_misery_bat(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (++o->timer > 16)
      {
        o->frame     = 2;
        o->invisible = false;
        FACEPLAYER;

        o->state  = 1;
        o->damage = 2;
        o->flags |= (FLAG_SHOOTABLE | FLAG_IGNORE_SOLID);

        o->ymark    = o->y;
        o->yinertia = 0x400;
      }
    }
    break;

    case 1:
    {
      ANIMATE(2, 0, 2);

      o->yinertia += (o->y < o->ymark) ? 0x40 : -0x40;
      XACCEL(0x10);

      if (o->x < 0 || o->x > MAPX(map.xsize) || o->y < 0 || o->y > MAPY(map.ysize))
      {
        o->Delete();
      }
    }
    break;
  }
}

void ai_misery_missile(Object *o)
{
  // cut & pasted from ai_x_fishy_missile
  vector_from_angle(o->angle, 0x400, &o->xinertia, &o->yinertia);
  int desired_angle = GetAngle(o->x, o->y, player->x, player->y);

  if (o->angle >= desired_angle)
  {
    if ((o->angle - desired_angle) < 128)
    {
      o->angle--;
    }
    else
    {
      o->angle++;
    }
  }
  else
  {
    if ((o->angle - desired_angle) < 128)
    {
      o->angle++;
    }
    else
    {
      o->angle--;
    }
  }

  // smoke trails
  if (++o->timer2 > 2)
  {
    o->timer2   = 0;
    Caret *c    = effect(o->ActionPointX(), o->ActionPointY(), EFFECT_SMOKETRAIL_SLOW);
    c->xinertia = -o->xinertia >> 2;
    c->yinertia = -o->yinertia >> 2;
  }

  o->frame = (o->angle + 16) / 32;
  if (o->frame > 7)
    o->frame = 7;
}

/*
void c------------------------------() {}
*/

void ai_sue_frenzied(Object *o)
{
  // AIDEBUG;
  sidekick_run_defeated(o, 500);

  switch (o->state)
  {
    case 0:
    {
      o->state       = 1;
      sue_being_hurt = sue_was_killed = false;

      o->savedhp = o->hp;
      o->nxflags |= NXFLAG_SLOW_X_WHEN_HURT;

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
      o->timer = 1;
    }
    case 1: // transforming
    {
      o->timer++;

      if (o->timer == 2)
      { // frenzied sue
        o->sprite = SPR_SUE_FRENZIED;
        o->frame  = 11;
        o->x -= 0x1000;
        o->y -= 0x1800;
      }

      if (o->timer == 4)
      { // normal sue
        o->timer = 0;

        o->sprite = SPR_SUE;
        o->frame  = 12;
        o->x += 0x1000;
        o->y += 0x1800;
      }

      if (++o->timer2 >= 50)
      {
        KillObjectsOfType(OBJ_RED_CRYSTAL);
        o->timer2 = 0;
        o->state  = 2;
      }
    }
    break;

    // fight begin/base state (script-triggered)
    case SUE_BASE:
    {
      o->state++;
      o->timer     = 0;
      o->frame     = 0;
      o->animtimer = 0;
      o->damage    = 0;

      o->flags |= FLAG_SHOOTABLE;
      o->flags &= ~FLAG_IGNORE_SOLID;
    }
    case SUE_BASE + 1:
    {
      ANIMATE(20, 0, 1);
      FACEPLAYER;

      o->xinertia *= 7;
      o->xinertia /= 8;
      o->yinertia *= 7;
      o->yinertia /= 8;

      if ((o->savedhp - o->hp) > 50)
      {
        o->savedhp     = o->hp;
        sue_being_hurt = true; // trigger Misery to spawn monsters
      }

      if (++o->timer > 80)
        o->state = SUE_PREPARE_ATTACK;
    }
    break;

    // prepare to attack
    case SUE_PREPARE_ATTACK:
    {
      o->state++;
      o->timer = 0;
      o->frame = 2;

      o->xinertia = 0;
      o->yinertia = 0;
    }
    case SUE_PREPARE_ATTACK + 1:
    {
      if (++o->timer > 16)
      {
        o->state = (o->timer2 ^= 1) ? SUE_SOMERSAULT : SUE_DASH;
        o->timer = 0;
      }
    }
    break;
  }

  sue_somersault(o);
  sue_dash(o);
}

// somersault attack. this is the only time she can actually hurt you.
static void sue_somersault(Object *o)
{
  switch (o->state)
  {
    case SUE_SOMERSAULT:
    {
      o->state++;
      o->timer  = 0;
      o->damage = 4;
      o->frame  = 2;

      FACEPLAYER;
      ThrowObjectAtPlayer(o, 0, 0x600);
      set_ignore_solid(o);
    }
    case SUE_SOMERSAULT + 1:
    {
      // passes through frame 3 (prepare/dash) before entering anim loop
      ANIMATE(1, 4, 7);
      o->timer++;

      if (o->shaketime && o->timer > 20)
      { // hurt fall
        o->state = SUE_SOMERSAULT_HIT;
        break;
      }

      // hit wall or timeout?
      if (o->timer > 50 || (o->blockr && o->xinertia > 0) || (o->blockl && o->xinertia < 0))
      { // back to base state
        o->state = SUE_BASE;
      }

      if ((o->timer % 5) == 1)
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CRITTER_FLY);
    }
    break;

    // hit during somersault
    case SUE_SOMERSAULT_HIT:
    {
      o->state++;
      o->timer  = 0;
      o->frame  = 2; // stop somersault; back to normal stand frame
      o->damage = 0;
      o->flags &= ~FLAG_IGNORE_SOLID;
    }
    case SUE_SOMERSAULT_HIT + 1: // slowing down
    {
      o->xinertia *= 7;
      o->xinertia /= 8;
      o->yinertia *= 7;
      o->yinertia /= 8;

      if (++o->timer > 6)
      {
        o->state++;
        o->timer    = 0;
        o->yinertia = -0x200;
        XMOVE(-0x200);
      }
    }
    break;

    // falling/egads
    case SUE_SOMERSAULT_HIT + 2:
    {
      o->frame = 9; // egads!

      if (o->blockd && o->yinertia > 0)
      {
        o->state++;
        o->timer = 0;
        o->frame = 2; // standing

        FACEPLAYER;
      }

      o->yinertia += 0x20;
      LIMITY(0x5ff);
    }
    break;

    // hit ground: slide a bit then recover
    case SUE_SOMERSAULT_HIT + 3:
    {
      if (++o->timer > 16)
        o->state = 20;
    }
    break;
  }
}

// non-harmful dash. she cannot be hurt, but cannot hurt you, either.
static void sue_dash(Object *o)
{
  int x;

  switch (o->state)
  {
    case SUE_DASH:
    {
      o->state++;
      o->timer = 0;

      FACEPLAYER;
      o->flags &= ~FLAG_SHOOTABLE;

      if (player->x < o->x)
        x = player->x - (160 * CSFI);
      else
        x = player->x + (160 * CSFI);

      ThrowObject(o, x, player->y, 0, 0x600);
      set_ignore_solid(o);
    }
    case SUE_DASH + 1:
    {
      // flash
      o->frame = (++o->timer & 2) ? 8 : 3; // frame 8 is invisible

      if (o->shaketime < 8)
        o->nxflags &= ~NXFLAG_SLOW_X_WHEN_HURT;

      if (o->timer > 50 || (o->blockr && o->xinertia > 0) || (o->blockl && o->xinertia < 0))
      {
        o->invisible = false;
        o->state     = SUE_BASE;
        o->nxflags |= NXFLAG_SLOW_X_WHEN_HURT;
      }
    }
    break;
  }
}

// sets FLAG_IGNORE_SOLID if the object is heading towards the center
// of the room, clears it otherwise.
static void set_ignore_solid(Object *o)
{
  int map_right_half  = ((map.xsize * TILE_W) * CSFI) / 2;
  int map_bottom_half = ((map.ysize * TILE_H) * CSFI) / 2;

  o->flags &= ~FLAG_IGNORE_SOLID;

  if ((o->x < map_right_half && o->xinertia > 0) || (o->x > map_right_half && o->xinertia < 0))
  {
    if ((o->y < map_bottom_half && o->yinertia > 0) || (o->y > map_bottom_half && o->yinertia < 0))
    {
      o->flags |= FLAG_IGNORE_SOLID;
    }
  }
}

/*
void c------------------------------() {}
*/

// shared between both Sue and Misery.
static void sidekick_run_defeated(Object *o, int health)
{
  // die if still around when core explodes
  if (o->state == SIDEKICK_CORE_DEFEATED_2)
  {
    if (!game.stageboss.object)
      o->hp = 0;
  }

  // trigger die
  if (o->hp < (1000 - health))
  {
    o->flags &= ~FLAG_SHOOTABLE;
    o->hp    = 9999; // don't re-trigger
    o->state = SIDEKICK_DEFEATED;
  }

  switch (o->state)
  {
    // the script triggers this if you defeat the core
    // without killing one or both sidekicks.
    //
    // once the core explodes and game.stageboss.object becomes NULL,
    // the sidekicks then enter the full defeated state and collapse.
    case SIDEKICK_CORE_DEFEATED:
    {
      if (o->hp == 9999)
      { // we were already dead when core was killed--ignore.
        o->state = SIDEKICK_DEFEATED + 1;
      }
      else
      {
        o->flags &= ~FLAG_SHOOTABLE;
        o->hp = 9999;

        o->xinertia = 0;
        o->yinertia = 0;
        o->frame    = 9;

        o->state = SIDEKICK_CORE_DEFEATED_2; // cannot "state++"; that is SIDEKICK_DEFEATED
      }
    }
    break;

    case SIDEKICK_DEFEATED:
    {
      o->state++;
      o->frame  = 9;
      o->damage = 0;
      o->flags &= ~FLAG_SHOOTABLE;
      o->flags |= FLAG_IGNORE_SOLID;

      o->yinertia = -0x200;
      o->shaketime += 50;

      if (o->type == OBJ_SUE_FRENZIED)
        sue_was_killed = true; // trigger Misery to start spawning missiles
      if (o->type == OBJ_MISERY_FRENZIED)
      {
        if (game.stageboss.object)
          game.stageboss.object->frame++; // trigger UDCore open
      }
    }
    case SIDEKICK_DEFEATED + 1:
    {
      o->yinertia += 0x20;

#define FLOOR (((13 * TILE_H) - 13) * CSFI)
      if (o->yinertia > 0 && o->y > FLOOR)
      {
        o->y = FLOOR;
        o->state++;
        o->frame    = 10;
        o->xinertia = 0;
        o->yinertia = 0;
      }
    }
    break;

    case SIDEKICK_CORE_DEFEATED_2:
      break;
  }
}
