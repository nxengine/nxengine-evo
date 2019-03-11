
// handle carets; a simplified type of objects used for visual effects.
// carets have no interaction with real objects, and are always drawn
// on top of all other objects and in front even of map foreground tiles.

#include "caret.h"

#include "autogen/sprites.h"
#include "common/llist.h"
#include "common/misc.h"
#include "common/stat.h"
#include "graphics/sprites.h"
#include "map.h"
#include "nx.h"
#include "trig.h"

#include <cmath>
using namespace Sprites;
#include "game.h"

Caret *firstcaret      = NULL;
Caret *lastcaret       = NULL;
static int _effecttype = EFFECT_NONE;

bool Carets::init(void)
{
  firstcaret = NULL;
  lastcaret  = NULL;
  return 0;
}

void Carets::close(void)
{
  Carets::DestroyAll();
}

/*
void c------------------------------() {}
*/

Caret *CreateCaret(int x, int y, int sprite, void (*ontick)(Caret *c), int xinertia, int yinertia)
{
  Caret *c = new Caret;
  memset(c, 0, sizeof(Caret));

  c->x          = x;
  c->y          = y;
  c->xinertia   = xinertia;
  c->yinertia   = yinertia;
  c->sprite     = sprite;
  c->OnTick     = ontick;
  c->effecttype = _effecttype;

  LL_ADD_END(c, prev, next, firstcaret, lastcaret);
  return c;
}

void Caret::Delete()
{
  this->deleted = true;
}

void Caret::Destroy()
{
  LL_REMOVE(this, prev, next, firstcaret, lastcaret);
  delete this;
}

void Caret::MoveAtDir(int dir, int speed)
{
  this->xinertia = 0;
  this->yinertia = 0;

  switch (dir)
  {
    case LEFT:
      this->xinertia = -speed;
      break;
    case RIGHT:
      this->xinertia = speed;
      break;
    case UP:
      this->yinertia = -speed;
      break;
    case DOWN:
      this->yinertia = speed;
      break;
  }
}

/*
void c------------------------------() {}
*/

void Carets::DrawAll(void)
{
  Caret *c = firstcaret;
  Caret *next;
  int scr_x, scr_y;

  while (c)
  {
    next = c->next;

    if (c->deleted)
    {
      c->Destroy();
    }
    else
    {
      // get caret's onscreen position
      // since caret's are all short-lived we just assume it's still onscreen
      // and let SDL's clipping handle it if not.
      if (!c->invisible && !c->deleted) // must check deleted again in case handler_function set it
      {
        scr_x = (c->x / CSFI) - (map.displayed_xscroll / CSFI);
        scr_y = (c->y / CSFI) - (map.displayed_yscroll / CSFI);
        scr_x -= sprites[c->sprite].frame[c->frame].dir[0].drawpoint.x;
        scr_y -= sprites[c->sprite].frame[c->frame].dir[0].drawpoint.y;

        drawSprite(scr_x, scr_y, c->sprite, c->frame, RIGHT);
      }
    }

    c = next;
  }
}

void Carets::UpdateAll(void)
{
  Caret *c = firstcaret;
  Caret *next;

  while (c)
  {
    next = c->next;

    if (c->deleted)
    {
      c->Destroy();
    }
    else
    {
      // do caret ai
      (*c->OnTick)(c);

      // move caret
      c->x += c->xinertia;
      c->y += c->yinertia;
    }

    c = next;
  }
}

int Carets::CountByEffectType(int type)
{
  int count = 0;
  Caret *c  = firstcaret;
  while (c)
  {
    if (c->effecttype == type)
      count++;
    c = c->next;
  }

  return count;
}

int Carets::DeleteByEffectType(int type)
{
  int count = 0;
  Caret *c  = firstcaret;
  while (c)
  {
    if (c->effecttype == type)
      c->Delete();
    c = c->next;
  }

  return count;
}

void Carets::DestroyAll(void)
{
  while (firstcaret)
    firstcaret->Destroy();
}

/*
void c------------------------------() {}
*/

// generates a caret-based effect at x, y. Most sprites used for carets have the
// drawpoint at their center so the effect is generally centered at that position.
//
// an effect can be just a convenience function for creating a caret
// with a particular sprite/ai combo, or it can produce a group of carets
// which are always seen together (e.g. bonkplus or bloodspatter).
Caret *effect(int x, int y, int effectno)
{
  Caret *c;
  int i;

  // tell CreateCaret what kind of effect we're spawning
  _effecttype = effectno;

  switch (effectno)
  {
    case EFFECT_STARSOLID:
      c = CreateCaret(x, y, SPR_STAR_SOLID, caret_animate3);
      break;
    case EFFECT_STARPOOF:
      c = CreateCaret(x, y, SPR_STAR_POOF, caret_animate3);
      break;
    case EFFECT_FISHY:
      c = CreateCaret(x, y, SPR_FISHY, caret_fishy);
      break;
    case EFFECT_BOOMFLASH:
      c = CreateCaret(x, y, SPR_BOOMFLASH, caret_animate3);
      break;
    case EFFECT_BUBBLE_BURST:
      c = CreateCaret(x, y, SPR_BUBBLE_BURST, caret_animate3);
      break;
    case EFFECT_SPUR_HIT:
      c = CreateCaret(x, y, SPR_SPUR_HIT, caret_spur_hit);
      break;
    case EFFECT_ZZZZ:
      c = CreateCaret(x, y, SPR_ZZZZ, caret_zzzz);
      break;
    case EFFECT_LEVELUP:
      c = CreateCaret(x, y, SPR_LEVELUP, caret_playertext);
      break;
    case EFFECT_LEVELDOWN:
      c = CreateCaret(x, y, SPR_LEVELDOWN, caret_playertext);
      break;
    case EFFECT_BONUSFLASH:
      c = CreateCaret(x, y, SPR_SMOKE_CLOUD, caret_bonusflash);
      break;
    case EFFECT_HEY:
      c = CreateCaret(x, y, SPR_HEY, caret_hey);
      break;
    case EFFECT_EMPTY:
      c = CreateCaret(x, y, SPR_EMPTY, caret_playertext);
      break;
    case EFFECT_SMOKETRAIL:
      c = CreateCaret(x, y, SPR_SMOKETRAIL, caret_animate2);
      break;

    case EFFECT_SMOKETRAIL_SLOW:
      c = CreateCaret(x, y, SPR_SMOKETRAIL, caret_animate3);
      break;

    case EFFECT_GUNFISH_BUBBLE:
    {
      c = CreateCaret(x - (3 * CSFI), y - (3 * CSFI), SPR_GUNFISH_BUBBLE, caret_gunfish_bubble);
      break;
    }
    break;

    case EFFECT_LAVA_SPLASH:
    {
      c = CreateCaret(x - (3 * CSFI), y - (3 * CSFI), SPR_LAVA_DRIP_SPLASH, caret_gunfish_bubble);
      break;
    }
    break;

    case EFFECT_GHOST_SPARKLE:
    {
      c           = CreateCaret(x, y, SPR_GHOST_SPARKLE, caret_ghost_sparkle);
      c->yinertia = random(-0x600, -0x200);
    }
    break;

    // "blood" spatters from shot hitting enemy
    case EFFECT_BLOODSPLATTER:
    {
      for (i = 0; i < 3; i++)
      {
        c = CreateCaret(x, y, SPR_BLOODHIT, caret_animate3);
        vector_from_angle(random(0, 255), (2 * CSFI), &c->xinertia, &c->yinertia);
      }
    }
    break;

    // two little blinky stars when player bonks his head on the ceiling
    case EFFECT_BONKPLUS:
    {
      for (i = 0; i < 2; i++)
      {
        c = CreateCaret(x, y, SPR_BONKHEADPLUS, caret_bonkplus);

        c->xinertia = random(-0x600, 0x600);
        c->yinertia = random(-0x200, 0x200);
        // uint8_t angle = random(-14, 14);
        // if (random(0, 1)) angle += 128;
        // vector_from_angle(angle, random(0x200, 0x384), &c->xinertia, &c->yinertia);
      }
    }
    break;

    case EFFECT_QMARK:
    {
      // only 1 question mark is ever shown at a time
      DeleteEffectsOfType(EFFECT_QMARK);
      c = CreateCaret(x, y, SPR_QMARK, caret_qmark);
    }
    break;

    default:
      staterr("effect: invalid effect type %d", effectno);
      return NULL;
  }

  _effecttype = EFFECT_NONE;
  return c;
}

/*
void c------------------------------() {}
*/

void caret_animate1(Caret *c)
{
  c->animdie(0);
}

void caret_animate2(Caret *c)
{
  c->animdie(1);
}

void caret_animate3(Caret *c)
{
  c->animdie(2);
}

void Caret::anim(int speed)
{
  Caret *const &c = this;

  if (++c->animtimer > speed)
  {
    c->animtimer = 0;

    if (++c->frame >= sprites[c->sprite].nframes)
      c->frame = 0;
  }
}

void Caret::animdie(int speed)
{
  Caret *const &c = this;

  if (++c->animtimer > speed)
  {
    c->animtimer = 0;

    if (++c->frame >= sprites[c->sprite].nframes)
      c->Delete();
  }
}

/*
void c------------------------------() {}
*/

// flickers rapidly and decels at exponential speed.
// used for the "bonkplus" effect when you bonk your head
void caret_bonkplus(Caret *c)
{
  c->xinertia *= 4;
  c->xinertia /= 5;
  c->yinertia *= 4;
  c->yinertia /= 5;

  c->invisible = (++c->timer & 2);

  if (c->timer > 20)
    c->Delete();
}

void caret_fishy(Caret *c)
{
  c->yinertia -= 16;
  c->animdie(4);
}

void caret_spur_hit(Caret *c)
{
  c->timer++;
  c->frame = (c->timer / 2) % 3;

  if (c->timer > 24)
    c->Delete();
}

// "Level Up", "Level Down", and "Empty" texts
void caret_playertext(Caret *c)
{
  int spd, stop;

  c->anim(1);

  // "EMPTY" text goes twice as fast as "Level" text
  if (c->sprite == SPR_EMPTY)
  {
    spd  = 2;
    stop = 18;
  }
  else
  {
    spd  = 1;
    stop = 20;
  }

  c->timer += spd;
  if (c->timer < 80)
  {
    if (c->timer < stop)
    {
      c->y -= (spd * CSFI);
    }
  }
  else
  {
    c->Delete();
  }
}

// ? effect when you press down with no object around to activate
void caret_qmark(Caret *c)
{
  if (++c->timer < 40)
  {
    if (c->timer < 7)
    {
      c->y -= (3 * CSFI);
    }
  }
  else
  {
    c->Delete();
  }
}

void caret_bonusflash(Caret *c)
{
  if (++c->timer == 4)
    c->Delete();
}

void caret_hey(Caret *c)
{
  if (++c->timer > 30)
    c->Delete();
  if (c->timer < 5)
    c->y -= (1 * CSFI);
}

void caret_gunfish_bubble(Caret *c)
{
  c->animdie(5);

  c->yinertia += 0x40;
  if (c->yinertia >= 0x5ff)
    c->yinertia = 0x5ff;
}

void caret_ghost_sparkle(Caret *c)
{
  c->invisible = (++c->timer & 2);

  if (c->timer > 20)
    c->Delete();
}

void caret_zzzz(Caret *c)
{
  c->animdie(5);

  c->x += 0x80;
  c->y -= 0x80;
}
