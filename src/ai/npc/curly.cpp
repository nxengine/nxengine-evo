#include "curly.h"

#include "../../ObjManager.h"
#include "../../game.h"
#include "../../map.h"
#include "../../p_arms.h"
#include "../../player.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

#define CURLY_STAND 0
#define CURLY_WALK 3
#define CURLY_WALKING 4

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_CURLY, ai_curly);

  AFTERMOVE(OBJ_CURLY_CARRIED, aftermove_curly_carried);

  ONTICK(OBJ_CURLY_CARRIED_SHOOTING, ai_curly_carried_shooting);
  ONTICK(OBJ_CCS_GUN, ai_ccs_gun);
}

/*
void c------------------------------() {}
*/

// regular NPC curly
void ai_curly(Object *o)
{
  switch (o->state)
  {
    case 0: // state 0: stand and do nothing
      o->frame = 0;
      o->flags |= FLAG_SCRIPTONACTIVATE; // needed for after Almond battle
    case 1:
      // important that state 1 does not change look-away frame for Drain cutscene
      if (o->frame != 12)
        o->frame = 0;
      o->xinertia = 0;
      break;

    case 3:  // state 3: walk forward
    case 10: // state 10: walk to player and stop
    {
      if (o->state == 10)
        FACEPLAYER;
      o->state++;
      o->animtimer = 0;
      o->frame     = 0;
    }
    case 4:
    case 11:
    {
      if (o->state == 11 && pdistlx(20 * CSFI))
      {
        o->state = 0;
        break;
      }

      ANIMATE(5, 0, 3);
      if (!o->blockd)
        o->frame = 3;

      XMOVE(0x200);
    }
    break;

    // state 5: curly makes a "kaboom", then looks sad.
    case 5:
      o->state = 6;
      SmokeClouds(o, 8, 0, 0);
    case 6:
      o->frame = 16;
      break;

    case 20: // face away
      o->xinertia = 0;
      o->frame    = 12;
      break;

    case 21: // look up
      o->xinertia = 0;
      o->frame    = 4;
      break;

    case 30: // state 30: curly goes flying through the air and is knocked out
    {
      o->state    = 31;
      o->frame    = 14;
      o->timer2   = 0;
      o->yinertia = -0x400;
      XMOVE(-0x200);
    }
    case 31:
    {
      if (o->blockd && o->yinertia >= 0)
        o->state = 32;
      else
        break;
    }
    case 32: // state 32: curly is laying knocked out
    {
      o->frame    = 15;
      o->xinertia = 0;
    }
    break;

    // walk backwards from collapsing wall during final cutscene
    case 70:
    {
      o->state     = 71;
      o->timer     = 0;
      o->frame     = 1;
      o->animtimer = 0;
    }
    case 71:
    {
      XMOVE(-0x100);
      ANIMATE(8, 0, 3);
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

// curly being carried by Tow Rope
void aftermove_curly_carried(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->frame = 17;
      o->flags &= ~FLAG_SCRIPTONACTIVATE;

      // turn on the HVTrigger in Waterway that kills Curly if you haven't
      // drained the water out of her
      if (game.curmap == STAGE_WATERWAY)
      {
        Object *t = FindObjectByID2(220);
        if (t)
          t->ChangeType(OBJ_HVTRIGGER);
      }
    }
    case 1:
    { // carried by player
      StickToPlayer(o, -2, -13, -18);
    }
    break;

    // floating away after Ironhead battle
    case 10:
    {
      o->xinertia = 0x40;
      o->yinertia = -0x20;
      o->state    = 11;
    }
    case 11:
    {
      if (o->y < MAPY(4)) // if in top part of screen, reverse before hitting wall
        o->yinertia = 0x20;
    }
    break;

    case 20:
    {
      o->Delete();
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

void ai_curly_carried_shooting(Object *o)
{
  if (o->state == 0)
  {
    o->x     = player->CenterX();
    o->y     = player->CenterY();
    o->state = 1;

    o->BringToFront();

    Object *gun;
    gun               = CreateObject(0, 0, OBJ_CCS_GUN);
    gun->linkedobject = o;
    gun->PushBehind(o);
  }

  // get player center position--
  // coordinates make more sense when figured this way
  int px = player->x + (8 * CSFI);
  int py = player->y + (8 * CSFI);

  o->dir = player->dir ^ 1;

  if (player->look)
  {
    o->xmark = px;

    if (player->look == UP)
    {
      if (player->blockd)
      {
        o->ymark = py - (12 * CSFI);
        o->frame = 1;
      }
      else
      {
        o->ymark = py + (8 * CSFI);
        o->frame = 2;
      }
    }
    else
    { // player looking down (and implicitly, not blockd)
      o->ymark = py - (8 * CSFI);
      o->frame = 1;
    }
  }
  else // normal/horizontal
  {
    if (player->dir == LEFT)
      o->xmark = px + (7 * CSFI);
    else
      o->xmark = px - (7 * CSFI);

    o->ymark = py - (3 * CSFI);
    o->frame = 0;
  }

  o->x += (o->xmark - o->x) / 2;
  o->y += (o->ymark - o->y) / 2;

  // bounce as player walks
  if (player->walking && (player->walkanimframe & 1))
    o->y -= (1 * CSFI);
}

void ai_ccs_gun(Object *o)
{
  Object *curly = o->linkedobject;
  if (!curly)
    return;

  o->dir   = curly->dir;
  o->frame = curly->frame;

  switch (o->frame)
  {
    case 0: // horizontal/normal
    {
      if (curly->dir == RIGHT)
        o->x = curly->x + (8 * CSFI);
      else
        o->x = curly->x - (8 * CSFI);

      o->y = curly->y;
    }
    break;

    case 1: // looking up
    {
      o->x = curly->x;
      o->y = curly->y - (10 * CSFI);
    }
    break;

    case 2: // looking down
    {
      o->x = curly->x;
      o->y = curly->y + (10 * CSFI);
    }
    break;
  }

  if (player->inputs_locked)
    return;

  if (pinputs[FIREKEY] != o->timer2)
  {
    o->timer2 = pinputs[FIREKEY];
    if (pinputs[FIREKEY])
    {
      if (CountObjectsOfType(OBJ_NEMESIS_SHOT_CURLY) < 2)
      {
        int shotdir = curly->dir;
        if (curly->frame == 1)
          shotdir = UP;
        if (curly->frame == 2)
          shotdir = DOWN;

        Object *shot = CreateObject(0, 0, OBJ_NEMESIS_SHOT_CURLY);
        SetupBullet(shot, curly->x, curly->y, B_CURLYS_NEMESIS, shotdir);
      }
    }
  }
}
