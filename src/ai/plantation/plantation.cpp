#include "plantation.h"

#include "../../ObjManager.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/graphics.h"
#include "../../map.h"
#include "../../player.h"
#include "../../slope.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../npc/npcregu.h"
#include "../sand/puppy.h"
#include "../stdai.h"
#include "../sym/smoke.h"
using namespace Graphics;
#include "../../autogen/sprites.h"
#include "../../graphics/sprites.h"
#include "../../graphics/tileset.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_ORANGEBELL, ai_orangebell);
  ONTICK(OBJ_ORANGEBELL_BABY, ai_orangebell_baby);

  ONTICK(OBJ_STUMPY, ai_stumpy);
  ONTICK(OBJ_MIDORIN, ai_midorin);

  ONTICK(OBJ_GUNFISH, ai_gunfish);
  ONTICK(OBJ_GUNFISH_SHOT, ai_gunfish_shot);

  ONTICK(OBJ_DROLL, ai_droll);
  ONTICK(OBJ_DROLL_SHOT, ai_droll_shot);
  ONTICK(OBJ_DROLL_GUARD, ai_droll_guard);

  ONTICK(OBJ_MIMIGA_FARMER_STANDING, ai_mimiga_farmer);
  ONTICK(OBJ_MIMIGA_FARMER_WALKING, ai_mimiga_farmer);

  ONTICK(OBJ_ROCKET, ai_rocket);
  ONTICK(OBJ_PROXIMITY_PRESS_HOZ, ai_proximity_press_hoz);

  ONTICK(OBJ_PUPPY_ITEMS, ai_puppy_wag);
  ONTICK(OBJ_NUMAHACHI, ai_numahachi);

  // nothing special is needed to keep them from leaving their cages
  // because the regular background tiles are set to be solid to NPC's.
  // Thus, they are no different from the mimiga farmer object.
  ONTICK(OBJ_MIMIGA_JAILED, ai_mimiga_farmer);
  ONSPAWN(OBJ_MIMIGA_CAGE, onspawn_mimiga_cage);

  GENERIC_NPC_NOFACEPLAYER(OBJ_MEGANE);
  GENERIC_NPC_NOFACEPLAYER(OBJ_CHIE);
  ONTICK(OBJ_ITOH, ai_npc_itoh);
  ONTICK(OBJ_KANPACHI_STANDING, ai_kanpachi_standing);
  ONTICK(OBJ_MOMORIN, ai_npc_momorin);
}

/*
void c------------------------------() {}
*/

// dragonfly creature
void ai_stumpy(Object *o)
{
  // debug("state: %d",o->state);
  // debug("timer: %d",o->timer);
  o->frame ^= 1;

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->flags |= NXFLAG_FOLLOW_SLOPE;
    }
    case 1:
    {
      if (pdistlx(240 * CSFI) && pdistly(192 * CSFI))
      {
        o->state = 2;
      }
    }
    break;

    case 2:
    {
      o->state = 3;
      o->timer = 0;
      FACEPLAYER;
    }
    case 3:
    {
      if (++o->timer > 50)
        o->state = 4;

      if (!pdistlx(SCREEN_WIDTH * CSFI) || !pdistly(SCREEN_HEIGHT * CSFI))
        o->state = 0;
    }
    break;

    case 4:
    {
      o->state = 5;
      o->timer = 0;

      // throw ourselves at player
      ThrowObjectAtPlayer(o, 3, 0x400);
      o->dir = (o->xinertia >= 0) ? RIGHT : LEFT;
    }
    case 5:
    {
      SIFSprite *sprite = &sprites[o->sprite];

      // don't bounce off slopes--some crap in here that should probably
      // be handled by the main engine somehow, maybe not setting blockl/r
      // for contact with slopes.
      if (o->xinertia < 0 && o->blockl)
      {
        if (++o->timer > 2 && !IsSlopeAtPointList(o, &sprite->block_l))
        {
          o->xinertia = -o->xinertia;
          o->dir ^= 1;
        }
      }
      else if (o->xinertia > 0 && o->blockr)
      {
        if (++o->timer > 2 && !IsSlopeAtPointList(o, &sprite->block_r))
        {
          o->xinertia = -o->xinertia;
          o->dir ^= 1;
        }
      }

      if (CheckStandOnSlope(o))
      {
        o->yinertia = -0x100;
      }
      else if (o->CheckAttribute(&sprite->block_d, TA_WATER))
      {
        o->yinertia = -0x200;
      }
      else if ((o->yinertia < 0 && o->blocku) || (o->yinertia >= 0 && o->blockd))
      {
        o->yinertia = -o->yinertia;
      }

      if (++o->timer > 50)
      {
        o->state    = 2;
        o->xinertia = 0;
        o->yinertia = 0;
      }
    }
    break;
  }
}

// very happy green guy who runs back and forth
void ai_midorin(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state    = 1;
      o->timer    = 0;
      o->frame    = 0;
      o->xinertia = 0;
    }
    case 1:
    {
      if (!random(0, 30))
      {
        o->state = 2 + random(0, 1);
        o->frame = 1;
      }
    }
    break;

    case 2: // blinking
    {
      if (++o->timer > 8)
      {
        o->state = 1;
        o->timer = 0;
        o->frame = 0;
      }
    }
    break;

    case 3: // running
    {
      o->state     = 4;
      o->frame     = 0; // this will be toggled into frame 2 just below
      o->animtimer = 0;

      o->timer = random(48, 64); // how long to run
      o->dir   = random(0, 1) ? LEFT : RIGHT;
    }
    case 4:
    {
      if (o->animtimer ^= 1)
        o->frame ^= 2;

      if ((o->dir == LEFT && o->blockl) || (o->dir == RIGHT && o->blockr))
      {
        o->dir ^= 1;
      }

      XMOVE(0x400);

      if (--o->timer <= 0)
        o->state = 0;
    }
    break;
  }

  o->yinertia += 0x20;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

// large bat with many baby bats
void ai_orangebell(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->state    = 1;
      o->ymark    = o->y;
      o->yinertia = 0x200;

      // create baby bats
      for (int i = 0; i < 8; i++)
      {
        Object *bat       = CreateObject(o->CenterX() + random(-0x1000, 0x1000), o->CenterY() + random(-0x1000, 0x1000),
                                   OBJ_ORANGEBELL_BABY);
        bat->linkedobject = o;
      }
    }
    case 1:
    {
      ANIMATE(5, 0, 2);

      if ((o->dir == LEFT && o->blockl) || (o->dir == RIGHT && o->blockr))
      {
        o->xinertia = 0;
        o->dir ^= 1;
      }

      XMOVE(0x100);

      o->yinertia += (o->y < o->ymark) ? 0x08 : -0x08;
      LIMITY(0x200);
    }
    break;
  }
}

void ai_orangebell_baby(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->xinertia = xinertia_from_angle(random(0, 255), 0x200);
      o->yinertia = yinertia_from_angle(random(0, 255), 0x200);
      o->flags |= FLAG_IGNORE_SOLID;

      o->timer  = 0;                             // time until can dive-bomb
      o->ymark2 = random(-32 * CSFI, 32 * CSFI); // unique target point on main bat

      o->state = 1;
    }
    case 1:
    {
      ANIMATE(1, 0, 2);

      if (o->linkedobject)
      {
        o->xmark = o->linkedobject->CenterX();
        o->ymark = o->linkedobject->CenterY() + o->ymark2;
        o->dir   = o->linkedobject->dir;
      }

      // DebugCrosshair(o->xmark, o->ymark, random(0,255),random(128,255),random(128,255));

      o->xinertia += (o->x < o->xmark) ? 0x08 : -0x08;
      o->yinertia += (o->y < o->ymark) ? 0x20 : -0x20;
      LIMITX(0x400);
      LIMITY(0x400);

      // dive-bomb
      if (o->timer)
        o->timer--;
      if (pdistlx(8 * CSFI) && !o->timer)
      {
        if (player->y > o->y && ((player->y - o->y) < 175 * CSFI))
        {
          o->xinertia /= 4;
          o->yinertia = 0;
          o->state    = 2;
          o->flags &= ~FLAG_IGNORE_SOLID;
        }
      }
    }
    break;

    case 2: // dive-bombing
    {
      o->frame = 3;
      o->yinertia += 0x40;
      LIMITY(0x5ff);

      if (o->blockd)
      {
        o->xinertia *= 2;
        o->yinertia = 0;
        o->timer    = 120;

        o->state = 1;
        o->flags |= FLAG_IGNORE_SOLID;
      }
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

void ai_gunfish(Object *o)
{
  if (player->Bottom() > o->y && player->Bottom() < o->Top() + (2 * CSFI))
  { // make sure platform is real solid for him
    o->flags &= ~FLAG_SOLID_MUSHY;
    o->flags |= FLAG_SOLID_BRICK;
  }
  else
  { // nice repel effects if he hits sides
    o->flags &= ~FLAG_SOLID_BRICK;
    o->flags |= FLAG_SOLID_MUSHY;
  }

  switch (o->state)
  {
    case 0:
    {
      o->yinertia = 0;
      o->ymark    = o->y;

      o->state = 1;
      o->timer = random(1, 50);
    }
    case 1: // desync
    {
      if (--o->timer <= 0)
      {
        o->yinertia = 0x200;
        o->state    = 2;
        o->timer    = 0;
      }
    }
    break;

    case 2:
    {
      ANIMATE(1, 0, 1);

      // this variation on FACEPLAYER helps keep them from turning
      // around so much when you are on standing on their platform.
      if (player->Right() < o->Left())
        o->dir = LEFT;
      if (player->Left() > o->Right())
        o->dir = RIGHT;

      if (pdistlx(128 * CSFI) && pdistly2(160 * CSFI, 20 * CSFI))
      {
        if (++o->timer > 80)
        {
          o->state = 10;
          o->timer = 0;
          o->frame += 2;
        }
      }
    }
    break;

    case 10:
    {
      ANIMATE(1, 2, 3);

      if (++o->timer > 20)
      {
        o->state = 20;
        o->timer = 0;
        o->frame += 2;
      }
    }
    break;

    case 20:
    {
      ANIMATE(1, 4, 5);

      if ((++o->timer % 10) == 3)
      {
        Object *shot = SpawnObjectAtActionPoint(o, OBJ_GUNFISH_SHOT);

        shot->xinertia = (o->dir == LEFT) ? -0x400 : 0x400;
        shot->yinertia = -0x400;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
      }

      if (o->timer > 60)
      {
        o->state = 2;
        o->timer = 0;
        o->frame -= 4;
      }
    }
    break;
  }

  o->yinertia += (o->y < o->ymark) ? 0x10 : -0x10;
  LIMITY(0x100);
}

void ai_gunfish_shot(Object *o)
{
  bool hit_something = false;

  if (o->xinertia <= 0 && o->blockl)
    hit_something = true;
  if (o->xinertia >= 0 && o->blockr)
    hit_something = true;
  if (o->yinertia <= 0 && o->blocku)
    hit_something = true;
  if (o->yinertia >= 0 && o->blockd)
    hit_something = true;

  if (++o->timer > 10)
  {
    SIFSprite *sprite = &sprites[o->sprite];
    if (o->CheckAttribute(&sprite->block_u, TA_WATER))
    {
      hit_something = true;
    }
  }

  if (hit_something)
  {
    for (int i = 0; i < 5; i++)
    {
      Caret *c    = effect(o->CenterX(), o->CenterY(), EFFECT_GUNFISH_BUBBLE);
      c->xinertia = random(-0x400, 0x400);
      c->yinertia = random(-0x400, 0);
    }

    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BUBBLE);
    o->Delete();
  }

  o->yinertia += 0x20;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_droll(Object *o)
{

  switch (o->state)
  {
    case 0:
    {
      o->xmark = o->x;
      o->state = 1;
    }
    case 1:
    {
      ANIMATE(40, 0, 1);
      FACEPLAYER;

      if (o->shaketime)
        o->state = 10;
    }
    break;

    case 10: // prepare to jump
    {
      o->state = 11;
      o->frame = 2;
      o->timer = 0;
    }
    case 11:
    {
      if (++o->timer > 10)
      { // jump
        o->state  = 12;
        o->frame  = 6;
        o->timer2 = 0; // have not fired yet

        o->xinertia = (o->x >= o->xmark) ? -0x200 : 0x200;
        o->yinertia = -0x600;
      }
    }
    break;

    case 12: // in air
    {
      if (o->yinertia >= 0)
      {
        if (!o->timer2) // have fired yet?
        {
          FACEPLAYER;
          o->timer2 = 1;
          o->frame  = 7;

          EmFireAngledShot(o, OBJ_DROLL_SHOT, 0, 0x600);
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_EM_FIRE);
        }
        else if (o->yinertia > 0x200)
        { // after-fire frame
          o->frame = 3;
        }

        if (o->blockd) // already know that yinertia >= 0
        {
          o->y /= CSFI;
          o->y *= CSFI;
          o->xinertia = 0;

          o->state = 13;
          o->frame = 2;
          o->timer = 0;
        }
      }
    }
    break;

    case 13: // landed
    {
      o->xinertia /= 2;

      if (++o->timer > 10)
      {
        o->xinertia = 0;
        o->state    = 1;
      }
    }
    break;
  }

  o->yinertia += 0x55;
  LIMITY(0x5ff);
}

void ai_droll_shot(Object *o)
{
  ANIMATE(0, 0, 2);

  if ((++o->timer % 5) == 0)
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DROLL_SHOT_FLY);

  bool hit_something = false;
  if (o->xinertia <= 0 && o->blockl)
    hit_something = true;
  if (o->xinertia >= 0 && o->blockr)
    hit_something = true;
  if (o->yinertia <= 0 && o->blocku)
    hit_something = true;
  if (o->yinertia >= 0 && o->blockd)
    hit_something = true;

  if (hit_something)
  {
    SmokeClouds(o, 4, 2, 2);
    effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);
    o->Delete();
  }
}

// stomps you in Teleporter Room
void ai_droll_guard(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->x += (8 * CSFI);
      o->state = 1;
    }
    case 1:
    {
      ANIMATE(30, 0, 1);
      FACEPLAYER;
    }
    break;

    case 10:
    {
      o->timer = 0;
      o->state = 11;
    }
    case 11:
    {
      o->frame = 2;
      if (++o->timer > 10)
      {
        o->state    = 12;
        o->frame    = 3;
        o->yinertia = -0x600;
        XMOVE(0x200);
      }
    }
    break;

    case 12: // in air...
    {
      if (o->yinertia >= 0 && o->blockd)
      {
        o->frame = 2;
        o->state = 13;
        o->timer = 0;
      }
    }
    break;

    case 13: // landed
    {
      o->xinertia /= 2;
      if (++o->timer > 10)
      {
        o->xinertia = 0;
        o->frame    = 0;
        o->state    = 1;
      }
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_mimiga_farmer(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->frame    = 0;
      o->xinertia = 0;
      o->state    = 1;
    }
    case 1:
    {
      if (!random(0, 60))
      {
        if (o->type != OBJ_MIMIGA_FARMER_STANDING && random(0, 1))
        { // walk
          o->state = 10;
        }
        else
        { // blink
          o->state = 2;
          o->frame = 1;
        }
      }
    }
    break;
    case 2: // blinking
    {
      if (++o->timer >= 8)
      {
        o->state = 0;
        o->timer = 0;
      }
    }
    break;

    case 10: // walk
    {
      o->state     = 11;
      o->frame     = 2;
      o->animtimer = 0;

      o->timer = random(16, 32);
      o->dir   = random(0, 1);
    }
    case 11:
    {
      if ((o->dir == LEFT && o->blockl) || (o->dir == RIGHT && o->blockr))
      {
        o->dir ^= 1;
      }

      XMOVE(0x200);
      ANIMATE(4, 2, 5);

      if (!--o->timer)
        o->state = 0;
    }
    break;
  }

  o->yinertia += 0x20;
  LIMITY(0x5ff);
}

void onspawn_mimiga_cage(Object *o)
{
  // ...because it isn't guaranteed that onspawn events
  // will only be called once during object creation.
  if (o->state == 1)
    return;
  o->state = 1;

  // object is used both in Jail2 and then reused in Ring2 (Doctor battle),
  // but tileset is different then.
  if (game.curmap == STAGE_KINGS_TABLE)
    o->sprite = SPR_MIMIGA_CAGE_FINAL;
  else
    o->sprite = SPR_JAIL2_CAGE;

  o->x -= (16 * CSFI);
}

/*
void c------------------------------() {}
*/

void ai_npc_itoh(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      // Option 2 is set when he is in plantation,
      // but he still should face left.
      o->dir   = LEFT;
      o->state = 1;
    }
    case 1:
    {
      o->xinertia = 0;
      o->frame    = 0;
      randblink(o);
    }
    break;

    case 10: // cower
    {
      o->frame    = 2;
      o->xinertia = 0;
    }
    break;

    case 20: // leap away
    {
      o->state    = 21;
      o->frame    = 2;
      o->xinertia = 0x200;
      o->yinertia = -0x400;
    }
    case 21:
    {
      if (o->blockd && o->yinertia >= 0)
      {
        o->xinertia = 0;

        o->frame = 3;
        o->state = 30;
        o->timer = 0;
      }
    }
    break;

    case 30: // shaking after leap
    {
      o->xmark = o->x;
      o->state = 31;
    }
    case 31:
    {
      o->x = o->xmark;
      if (++o->timer & 2)
        o->x += (1 * CSFI);
    }
    break;

    case 40: // small vertical hop (from when he jumps out of helicopter at end)
    {
      o->state    = 41;
      o->yinertia = -0x200;
      o->frame    = 2;
    }
    case 41:
    {
      if (o->blockd && o->yinertia > 0)
      {
        o->xinertia = 0;
        o->state    = 42;
        o->frame    = 0;
        o->dir      = RIGHT;
      }
    }
    break;
    case 42: // stop begin shocked after blocks fall (same as 0 except he doesn't blink)
    {
      o->frame = 0;
      o->dir   = RIGHT;
    }
    break;

    case 50: // run a moment then stop
    {
      o->state     = 51;
      o->timer     = 0;
      o->animtimer = 0;
      o->dir       = RIGHT;
      o->frame     = 5;
    }
    case 51:
    {
      ANIMATE(3, 4, 7);
      XMOVE(0x200);

      if (++o->timer > 32)
      {
        o->frame    = 0;
        o->xinertia = 0;
        o->state    = 52;
      }
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}

void ai_kanpachi_standing(Object *o)
{
  switch (o->state)
  {
    case 10: // walking
    case 11:
    {
      npc_generic_walk(o, 10);
    }
    break;

    case 20: // face away/enter door
    {
      o->xinertia = 0;
      o->frame    = 6;
    }
    break;

    default:
      ai_generic_npc_nofaceplayer(o);
      break;
  }
}

void ai_npc_momorin(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->frame = 0;
      randblink(o);
      FACEPLAYER;
      break;

    case 3: // surprised
      o->frame = 2;
      break;
  }
}

/*
void c------------------------------() {}
*/

void ai_proximity_press_hoz(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (o->dir == LEFT)
        o->x -= (8 * CSFI);
      o->xmark = o->x;

      o->state = 1;
    }
    case 1:
    {
      if (pdistly2(0x800, 0x1000))
      {
        if (o->dir == LEFT)
        {
          if (player->Right() <= o->x)
          {
            if ((o->x - player->Right()) <= (192 * CSFI))
            {
              o->state = 2;
              o->frame = 2;
              o->timer = 0;
            }
          }
        }
        else
        {
          if (player->x >= o->x)
          {
            if ((player->x - o->x) <= (192 * CSFI))
            {
              o->state = 2;
              o->frame = 2;
              o->timer = 0;
            }
          }
        }
      }
    }
    break;

    case 2: // activated
    {
      o->damage = 127;
      XMOVE(0xC00);

      if (++o->timer == 8)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
        SmokeSide(o, 4, o->dir);
      }

      if (o->timer > 8)
      {
        o->damage   = 0;
        o->xinertia = 0;
        o->state    = 3;
        o->timer    = 0;
      }
    }
    break;

    case 3: // hit other press
    {
      if (++o->timer > 50)
      {
        o->state = 4; // return
        o->frame = 1;
        o->timer = 0;
      }
    }
    break;

    case 4: // return to start pos
    {
      XMOVE(-0x800);

      if (++o->timer > 10)
      {
        o->frame    = 0;
        o->xinertia = 0;
        o->x        = o->xmark;

        o->state = 1;
        o->timer = 0;
      }
    }
    break;
  }
}

/*
void c------------------------------() {}
*/

static Caret *SpawnRocketTrail(Object *o, int side)
{
  Caret *trail;
  int x, y;

  if (side == LEFT)
  {
    x = o->ActionPointX();
    y = o->ActionPointY();
  }
  else
  {
    x = o->ActionPoint2X();
    y = o->ActionPoint2Y();
  }

  trail           = effect(x, y, EFFECT_SMOKETRAIL);
  trail->yinertia = 0x400;
  return trail;
}

void ai_rocket(Object *o)
{

  switch (o->state)
  {
    case 0:
      break;

    case 10: // activated
    {
      o->state = 11;
      o->timer = 0;
    }
    case 11:
    {
      o->timer++;
      o->yinertia += 8;

      if (o->blockd)
      {
        if (o->timer < 10)
        {
          o->state = 12;
        }
        else
        {
          o->state = 0;
        }
      }
    }
    break;

    case 12:
    {
      o->flags &= ~FLAG_SCRIPTONACTIVATE;
      o->state = 13;
      o->timer = 0;
      o->frame = 1;

      SmokeClouds(o, 10, 16, 8);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
    }
    case 13:
    {
      o->yinertia -= 0x08;

      // boosters fire left/right alternating:
      o->timer++;
      SpawnRocketTrail(o, (o->timer & 1) ? RIGHT : LEFT);

      // noise
      if ((o->timer % 4) == 1)
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);

      // smacked into ceiling. note that I didn't check that we're actually
      // riding the rocket when I test player->blocku--the original doesn't
      // seem to either, so I'm emulating that bug.
      if (o->blocku || player->blocku)
      {
        if (player->cannotride == o)
          player->cannotride = NULL;

        if (!player->blocku && (player->riding == o || player->lastriding == o))
        {
          if (o->yinertia < 0 && o->yinertia < player->yinertia)
            player->yinertia = o->yinertia;
        }

        SmokeClouds(o, 6, 16, 8);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);

        o->yinertia = 0;
        o->state    = 14;
      }
    }
    break;

    case 14: // falling back down
    {
      o->yinertia += 8;
      o->timer++;

      if (o->yinertia < 0)
      {
        if ((o->timer % 4) == 0)
          SpawnRocketTrail(o, (o->timer & 8) ? RIGHT : LEFT);

        // sound
        if ((o->timer % 16) == 1)
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);
      }
      else if (o->blockd)
      {
        o->y /= CSFI;
        o->y *= CSFI;
        o->flags |= FLAG_SCRIPTONACTIVATE;
        o->frame = 0;
        o->state = 0;
      }
    }
    break;
  }

  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

// in the Statue Room, if you enter via the Plantation
void ai_numahachi(Object *o)
{
  switch (o->state)
  {
    case 0:
    case 1:
    {
      o->xinertia = 0;
      o->state    = 2;
    }
    case 2:
    {
      ANIMATE(50, 0, 1);
    }
    break;
  }

  o->yinertia += 0x40;
  LIMITY(0x5ff);
}
