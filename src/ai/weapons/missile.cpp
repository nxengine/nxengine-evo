#include "missile.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "weapons.h"

#define STATE_WAIT_RECOIL_OVER 1
#define STATE_RECOIL_OVER 2
#define STATE_MISSILE_CAN_EXPLODE 3

struct MissileSettings
{
  int maxspeed;   // max speed of missile
  int hitrange;   //
  int lifetime;   // number of boomflashes to create on impact
  int boomrange;  // max dist away to create the boomflashes
  int boomdamage; // damage dealt by contact with a boomflash (AoE damage)
} missile_settings[] = {
    //  Level 1-3 regular missile
    //  maxspd   hit,  life, range,  bmdmg
    {0xA00, 16, 10, 16, 1},
    {0xA00, 16, 15, 32, 1},
    {0xA00, 16, 5, 40, 1},

    //  Level 1-3 super missile
    //  maxspd   hit,  life, range, bmdmg
    {0x1400, 12, 10, 16, 2},
    {0x1400, 12, 14, 32, 2},
    {0x1400, 12, 6, 40, 2}};

INITFUNC(AIRoutines)
{
  AFTERMOVE(OBJ_MISSILE_SHOT, ai_missile_shot);
  AFTERMOVE(OBJ_SUPERMISSILE_SHOT, ai_missile_shot);
  ONTICK(OBJ_MISSILE_BOOM_SPAWNER, ai_missile_boom_spawner_tick);
  AFTERMOVE(OBJ_MISSILE_BOOM_SPAWNER, ai_missile_boom_spawner);
}

/*
void c------------------------------() {}
*/

void ai_missile_shot(Object *o)
{
  int index                 = o->shot.level + ((o->type == OBJ_SUPERMISSILE_SHOT) ? 3 : 0);
  MissileSettings *settings = &missile_settings[index];

  if (o->state == 0)
  {
    o->shot.damage = 0;

    if (o->shot.level == 2)
    {
      // initilize wavey effect
      o->xmark = o->x;
      o->ymark = o->y;
      o->speed = (o->type == OBJ_SUPERMISSILE_SHOT) ? -64 : -32;

      // don't let it explode until the "recoil" effect is over.
      o->state = STATE_WAIT_RECOIL_OVER;
      // record position we were fired at (we won't explode until we pass it)
      o->xmark2 = player->x;
      o->ymark2 = player->y;
    }
    else
    {
      o->state = STATE_MISSILE_CAN_EXPLODE;
    }
  }

  // accelerate according to current type and level of missile
  // don't use LIMITX here as it can mess up recoil of level 3 super missiles
  switch (o->shot.dir)
  {
    case RIGHT:
      o->xinertia += o->shot.accel;
      if (o->xinertia > settings->maxspeed)
        o->xinertia = settings->maxspeed;
      break;

    case LEFT:
      o->xinertia -= o->shot.accel;
      if (o->xinertia < -settings->maxspeed)
        o->xinertia = -settings->maxspeed;
      break;

    case UP:
      o->yinertia -= o->shot.accel;
      if (o->yinertia < -settings->maxspeed)
        o->yinertia = -settings->maxspeed;
      break;

    case DOWN:
      o->yinertia += o->shot.accel;
      if (o->yinertia > settings->maxspeed)
        o->yinertia = settings->maxspeed;
      break;
  }

  // wavey effect for level 3
  // (markx/y is used as a "speed" value here)
  if (o->shot.level == 2)
  {
    if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
    {
      if (o->y >= o->ymark)
        o->speed = (o->type == OBJ_SUPERMISSILE_SHOT) ? -64 : -32;
      else
        o->speed = (o->type == OBJ_SUPERMISSILE_SHOT) ? 64 : 32;
      o->yinertia += o->speed;
    }
    else
    {
      if (o->x >= o->xmark)
        o->speed = (o->type == OBJ_SUPERMISSILE_SHOT) ? -64 : -32;
      else
        o->speed = (o->type == OBJ_SUPERMISSILE_SHOT) ? 64 : 32;
      o->xinertia += o->speed;
    }
  }

  // check if we hit an enemy
  // level 3 missiles can not blow up while they are "recoiling"
  // what we do is first wait until they're traveling in the direction
  // they're pointing, then wait till they pass the player's original position.
  switch (o->state)
  {
    case STATE_WAIT_RECOIL_OVER:
      switch (o->shot.dir)
      {
        case LEFT:
          if (o->xinertia <= 0)
            o->state = STATE_RECOIL_OVER;
          break;
        case RIGHT:
          if (o->xinertia >= 0)
            o->state = STATE_RECOIL_OVER;
          break;
        case UP:
          if (o->yinertia <= 0)
            o->state = STATE_RECOIL_OVER;
          break;
        case DOWN:
          if (o->yinertia >= 0)
            o->state = STATE_RECOIL_OVER;
          break;
      }
      if (o->state != STATE_RECOIL_OVER)
        break;

    case STATE_RECOIL_OVER:
      switch (o->shot.dir)
      {
        case LEFT:
          if (o->x <= o->xmark2 - (2 * CSFI))
            o->state = STATE_MISSILE_CAN_EXPLODE;
          break;
        case RIGHT:
          if (o->x >= o->xmark2 + (2 * CSFI))
            o->state = STATE_MISSILE_CAN_EXPLODE;
          break;
        case UP:
          if (o->y <= o->ymark2 - (2 * CSFI))
            o->state = STATE_MISSILE_CAN_EXPLODE;
          break;
        case DOWN:
          if (o->y >= o->ymark2 + (2 * CSFI))
            o->state = STATE_MISSILE_CAN_EXPLODE;
          break;
      }
      if (o->state != STATE_MISSILE_CAN_EXPLODE)
        break;

    case STATE_MISSILE_CAN_EXPLODE:
    {
      bool blow_up = false;

      if (damage_enemies(o))
      {
        blow_up = true;
      }
      else
      { // check if we hit a wall
        if (o->shot.dir == LEFT && o->blockl)
          blow_up = true;
        else if (o->shot.dir == RIGHT && o->blockr)
          blow_up = true;
        else if (o->shot.dir == UP && o->blocku)
          blow_up = true;
        else if (o->shot.dir == DOWN && o->blockd)
          blow_up = true;
      }

      if (blow_up)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MISSILE_HIT);

        // create the boom-spawner object for the flashes, smoke, and AoE damage
        int y = o->CenterY();
        if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
          y -= 3 * CSFI;
        Object *sp = CreateBullet(o->CenterX(), y, OBJ_MISSILE_BOOM_SPAWNER);

        sp->shot.boomspawner.range      = settings->hitrange;
        sp->shot.boomspawner.booms_left = settings->lifetime;
        sp->shot.damage                 = settings->boomdamage;
        sp->shot.level                  = settings->boomdamage;

        o->Delete();
        return;
      }
    }
    break;
  }

  if (--o->shot.ttl < 0)
    shot_dissipate(o, EFFECT_STARPOOF);

  // smoke trails
  if (++o->timer > 2)
  {
    o->timer     = 0;
    Caret *trail = effect(o->CenterX() - o->xinertia, o->CenterY() - o->yinertia, EFFECT_SMOKETRAIL);

    const int trailspd = 0x400;
    switch (o->shot.dir)
    {
      case LEFT:
        trail->xinertia = trailspd;
        trail->y -= (2 * CSFI);
        break;
      case RIGHT:
        trail->xinertia = -trailspd;
        trail->y -= (2 * CSFI);
        break;
      case UP:
        trail->yinertia = trailspd;
        trail->x -= (1 * CSFI);
        break;
      case DOWN:
        trail->yinertia = -trailspd;
        trail->x -= (1 * CSFI);
        break;
    }
  }
}

void ai_missile_boom_spawner(Object *o)
{

  if (o->state == 0)
  {
    o->state = 1;
    o->timer = 0;

    o->xmark = o->x;
    o->ymark = o->y;

    // give us the same bounding box as the boomflash effects
    o->sprite    = SPR_BOOMFLASH;
    o->invisible = true;
  }

  if (!(o->shot.boomspawner.booms_left % 3))
  {
    int range = 0;
    switch (o->shot.level)
    {
      case 1:
        range = 16;
        break;
      case 2:
        range = 32;
        break;
      case 3:
        range = 40;
        break;
    }
    int x = o->CenterX() + (random(-range, range) * CSFI);
    int y = o->CenterY() + (random(-range, range) * CSFI);

    effect(x, y, EFFECT_BOOMFLASH);
    missilehitsmoke(x, y, o->shot.boomspawner.range);
  }

  if (--o->shot.boomspawner.booms_left < 0)
    o->Delete();
}

void ai_missile_boom_spawner_tick(Object *o)
{
  damage_all_enemies_in_bb(o, FLAG_INVULNERABLE, o->CenterX(), o->CenterY(), o->shot.boomspawner.range);
}

static void missilehitsmoke(int x, int y, int range)
{
  int smokex = x + (random(-range, range) * CSFI);
  int smokey = y + (random(-range, range) * CSFI);
  Object *smoke;

  for (int i = 0; i < 2; i++)
  {
    smoke         = CreateObject(smokex, smokey, OBJ_SMOKE_CLOUD);
    smoke->sprite = SPR_MISSILEHITSMOKE;
    vector_from_angle(random(0, 255), random(0x100, 0x3ff), &smoke->xinertia, &smoke->yinertia);
  }
}
