#include "fireball.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../game.h"
#include "../../graphics/Renderer.h"
#include "../../sound/SoundManager.h"
#include "weapons.h"

using namespace NXE::Graphics;

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_FIREBALL1, ai_fireball);
  ONTICK(OBJ_FIREBALL23, ai_fireball_level_23);

  ONTICK(OBJ_FIREBALL_TRAIL, ai_fireball_trail);
}

/*
void c------------------------------() {}
*/

// shared between all weapon levels
void ai_fireball(Object *o)
{
  if (--o->shot.ttl < 0)
  {
    shot_dissipate(o, EFFECT_STARPOOF);
    return;
  }

  // bounce
  if (o->blockl || o->blockr || o->blockd)
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);

    if (o->blockd)
      o->yinertia = -0x400;
    if (o->blockl)
      o->xinertia = 0x400;
    if (o->blockr)
      o->xinertia = -0x400;

    // dissipate immediately if we somehow get entirely embedded in a wall.
    if (o->blockl && o->blockr && o->blocku && o->blockd)
    {
      shot_dissipate(o, EFFECT_STARPOOF);
      return;
    }
  }

  // fall
  o->yinertia += 0x55;
  if (o->yinertia > 0x3ff)
    o->yinertia = 0x3ff;

  // hit ceil
  if (o->blocku && o->yinertia < 0)
    o->yinertia = 0;

  // spin in shot direction
  if (o->dir == RIGHT)
  {
    if (++o->frame >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
      o->frame = 0;
  }
  else
  {
    if (--o->frame < 0)
      o->frame = Renderer::getInstance()->sprites.sprites[o->sprite].nframes - 1;
  }

  Object *enemy;
  if ((enemy = check_hit_enemy(o)))
  {
    // bounce off of invulnerable non-enemy objects instead of dissipating
    // (prevents incorrect dissipation if a fireball hits the Lift in Almond)
    if ((enemy->flags & FLAG_INVULNERABLE) && enemy->damage == 0)
    {
      static const Point embedpt[] = {{8, 8}};
      static const Point pcheckl[] = {{-1, 4}, {-1, 12}};
      static const Point pcheckr[] = {{16, 4}, {16, 12}};
      static const Point pchecku[] = {{4, -1}, {12, -1}};
      static const Point pcheckd[] = {{4, 16}, {12, 16}};

      // keeps us from bouncing around forever if something gets screwed up
      if (!o->CheckSolidIntersect(enemy, embedpt, 1))
      {
        // figure out whether we're on top, to the left, or etc of whatever we hit
        // so we know which way to bounce off
        if (o->CheckSolidIntersect(enemy, pcheckl, 2))
        {
          o->xinertia = 0x400;
          return;
        }

        if (o->CheckSolidIntersect(enemy, pcheckr, 2))
        {
          o->xinertia = -0x400;
          return;
        }

        if (o->CheckSolidIntersect(enemy, pchecku, 2))
        {
          o->yinertia = 0x400;
          return;
        }

        if (o->CheckSolidIntersect(enemy, pcheckd, 2))
        {
          o->yinertia = -0x400;
          return;
        }
      }

      shot_dissipate(o, EFFECT_STARPOOF);
      return;
    }

    // "Fireball will barrel through weaker enemies, dealing only half damage
    //  and continuing on if the enemy is killed by that much damage."
    // So, our damage is set to half of what it should be, and we'll let ourselves
    // deal it up to twice.
    enemy->DealDelayedDamage(o->shot.damage);

    if (o->timer3 == 1)
      shot_dissipate(o, EFFECT_STARPOOF);
    else
      o->timer3 = 1;
  }
}

void ai_fireball_level_23(Object *o)
{
  // everything from level 1, and...
  ai_fireball(o);
  if (o->deleted)
    return;

  // ...spawn trails
  if ((++o->timer2 & 1) == 0)
  {
    create_fire_trail(o, OBJ_FIREBALL_TRAIL, o->shot.level);
  }
}

/*
void c------------------------------() {}`
*/

// shared between Fireball and Snake
Object *create_fire_trail(Object *o, int objtype, int level)
{
  Object *trail = CreateObject(o->x, o->y, objtype);

  trail->sprite = (level == 2) ? SPR_FIREBALL_TRAIL3 : SPR_FIREBALL_TRAIL2;
  trail->dir    = o->dir;

  // be sure and show the 1st frame
  trail->animframe = -1;

  // draw the trail BEHIND the lead fireball
  trail->PushBehind(o);

  return trail;
}

void ai_fireball_trail(Object *o)
{
  static const int ccw_trail_frames[] = {2, 1, 0, 5, 4, 3, 8, 7, 6};

  if (++o->animframe >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
  {
    o->Delete();
  }
  else
  {
    if (o->dir == RIGHT)
    {
      o->frame = o->animframe;
    }
    else
    {
      o->frame = ccw_trail_frames[o->animframe];
    }
  }
}
