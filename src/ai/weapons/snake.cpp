#include "snake.h"

#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Renderer.h"
#include "../../object.h"
#include "fireball.h"
#include "weapons.h"

using namespace NXE::Graphics;

// settings for wavy and acceleration of levels 2 & 3
#define SNAKE_VERT_SPEED 0x400
#define SNAKE_ACCEL_SPEED 0x80

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_SNAKE1_SHOT, ai_snake);

  ONTICK(OBJ_SNAKE23_SHOT, ai_snake_23);
  ONTICK(OBJ_SNAKE_TRAIL, ai_snake_trail);
}

/*
void c------------------------------() {}
*/

void ai_snake(Object *o)
{
  if (o->state == 0)
  {
    o->state = 1;
    o->flags |= FLAG_IGNORE_SOLID;

    o->frame = random(0, 2);
  }

  if (--o->shot.ttl < 0)
  {
    // emphasis wave effect
    if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
      o->y += o->yinertia;
    else
      o->x += o->xinertia;

    shot_dissipate(o, EFFECT_STARPOOF);
    return;
  }

  // spin in shot direction
  if (o->shot.dir == LEFT)
  {
    if (--o->frame < 0)
      o->frame = Renderer::getInstance()->sprites.sprites[o->sprite].nframes - 1;
  }
  else
  {
    if (++o->frame >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
      o->frame = 0;
  }

  if (damage_enemies(o))
  {
    shot_dissipate(o, EFFECT_STARPOOF);
  }
}

void ai_snake_23(Object *o)
{
  static int wave_dir = 0;

  if (o->state == 0)
  {
    // start moving off at an angle to our direction.
    // whether we start off going up or down alternates with each shot.
    int wavespeed = (wave_dir & 1) ? -SNAKE_VERT_SPEED : SNAKE_VERT_SPEED;
    wave_dir ^= 1;

    if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
    {
      o->yinertia = wavespeed;
    }
    else
    {
      o->xinertia = wavespeed;
    }

    // ...we don't set state to 1, that'll be done in ai_snake common code
  }
  else
  {
    // accelerate the shot
    switch (o->shot.dir)
    {
      case LEFT:
        o->xinertia -= 0x80;
        break;
      case UP:
        o->yinertia -= 0x80;
        break;
      case RIGHT:
        o->xinertia += 0x80;
        break;
      case DOWN:
        o->yinertia += 0x80;
        break;
    }
  }

  // periodically abruptly change the wave's direction
  if ((++o->timer % 5) == 2)
  {
    if (o->shot.dir == LEFT || o->shot.dir == RIGHT)
    {
      o->yinertia = -o->yinertia;
    }
    else
    {
      o->xinertia = -o->xinertia;
    }
  }

  // spawn trails
  Object *trail    = create_fire_trail(o, OBJ_SNAKE_TRAIL, o->shot.level);
  trail->frame     = random(0, 2);
  trail->animtimer = 0;
  trail->yinertia  = -0x200;

  // ... and all the basic logic from level 1
  ai_snake(o);
}

/*
void c------------------------------() {}
*/

void ai_snake_trail(Object *o)
{
  if (++o->animtimer > 1)
  {
    o->animtimer = 0;
    o->frame += 3;

    if (o->frame >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
      o->Delete();
  }
}
