#include "weed.h"

#include "../../autogen/sprites.h"
#include "../../caret.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_CRITTER_FLYING, ai_critter);
  ONTICK(OBJ_POWER_CRITTER, ai_critter);

  ONTICK(OBJ_BAT_HANG, ai_bat_hang);
  ONTICK(OBJ_BAT_CIRCLE, ai_bat_circle);

  ONTICK(OBJ_JELLY, ai_jelly);
  ONTICK(OBJ_GIANT_JELLY, ai_giant_jelly);

  ONTICK(OBJ_MANNAN, ai_mannan);
  ONTICK(OBJ_MANNAN_SHOT, ai_mannan_shot);

  ONTICK(OBJ_FROG, ai_frog);
  ONTICK(OBJ_MINIFROG, ai_frog);

  ONTICK(OBJ_SANTAS_KEY, ai_animate2);

  ONTICK(OBJ_HEY_SPAWNER, ai_hey_spawner);
  ONTICK(OBJ_MOTORBIKE, ai_motorbike);

  ONTICK(OBJ_POWERCOMP, ai_animate3);
  ONTICK(OBJ_POWERSINE, ai_animate1);
  ONTICK(OBJ_MALCO, ai_malco);
  ONTICK(OBJ_MALCO_BROKEN, ai_malco_broken);
}

/*
void c------------------------------() {}
*/

void ai_critter(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      if (o->type == OBJ_POWER_CRITTER)
      { // altered physics for Power Critter
        o->critter.jumpheight = 0x2800;
        o->critter.jumpgrav   = 0x1C;
        o->critter.falldmg    = 12;
        o->critter.canfly     = true;
      }
      else
      {
        o->critter.jumpheight = 0;
        o->critter.jumpgrav   = 0x40;
        o->critter.falldmg    = 3;

        if (o->type == OBJ_CRITTER_HOPPING_BLUE ||  // first cave
            o->type == OBJ_CRITTER_HOPPING_GREEN || // egg 1
            o->type == OBJ_CRITTER_HOPPING_AQUA ||  // egg 2
            o->type == OBJ_CRITTER_HOPPING_RED)     // last cave
        {
          o->critter.canfly = false;

          // critters in egg1 only 2 dmg
          if (o->type == OBJ_CRITTER_HOPPING_GREEN)
            o->critter.falldmg = 2;

          // critters in First Cave deal 1 damage
          if (o->type == OBJ_CRITTER_HOPPING_BLUE)
            o->critter.falldmg = 1;

          // critters in First Cave don't jump as high
          if (o->type != OBJ_CRITTER_HOPPING_BLUE)
          {
            o->critter.jumpgrav = 0x2C;
          }
        }
        else
        {
          // critters are purple in Maze
          o->sprite = SPR_CRITTER_FLYING_CYAN;

          o->critter.canfly = true;
        }
      }

      o->state = 1;
    } // fall thru
    case 1:
    {
      o->frame = 0;

      if (o->timer >= 8)
      {
        int attack_dist = (o->critter.canfly) ? (96 * CSFI) : (64 * CSFI);

        // close enough to attack?
        if (pdistlx(attack_dist) && pdistly2(96 * CSFI, 48 * CSFI))
        {
          o->state = 2;
          o->frame = 0;
          o->timer = 0;
        }
        else if (pdistlx(attack_dist + (32 * CSFI)) && pdistly2(128 * CSFI, 48 * CSFI))
        { // no, but close enough to look up at "attention" and watch player
          FACEPLAYER;
          o->frame = 1;
          o->timer = 8; // reset timer to stop watching
        }
        else
        {
          // once a little time has passed stop watching him if he turns his back
          if ((o->x > player->x && player->dir == LEFT) || (o->x < player->x && player->dir == RIGHT))
          {
            if (++o->timer >= 150)
            {
              o->frame = 0;
              o->timer = 8;
            }
          }
          else
            o->timer = 8;
        }
      }
      else
      {
        o->timer++;
      }

      if (o->shaketime)
      { // attack if shot
        o->state = 2;
        o->frame = 0;
        o->timer = 0;
      }
    }
    break;

    case 2: // start jump
    {
      if (++o->timer > 8)
      {
        o->state    = 3;
        o->frame    = 2;
        o->yinertia = -1228;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);

        FACEPLAYER;
        XMOVE(0x100);
      }
    }
    break;

    case 3: // jumping
    {
      // enter flying phase as we start to come down or
      // if we hit the ceiling.
      if (o->yinertia > 0x100 && !o->blocku)
      {
        // during flight we will sine-wave oscilliate around this position
        o->ymark = (o->y - o->critter.jumpheight);

        o->state = 4;
        o->frame = 3;
        o->timer = 0;
      }
      else
      {
        if (o->blockd && o->yinertia >= 0)
        { // jumped onto a platform before we got to fly--land immediately
          goto landed;
        }
        break;
      }
    }       // fall-thru
    case 4: // flying
    {
      FACEPLAYER;

      // time to come down yet?
      // (come down immediately if we are not one of the flying critters)
      if (!o->critter.canfly || o->blockl || o->blockr || o->blocku || ++o->timer > 100)
      {
        o->damage = o->critter.falldmg; // increased damage if falls on player
        o->state  = 5;
        o->frame  = 2;
        o->yinertia /= 2;
      }
      else
      {
        // run the propeller
        ANIMATE(0, 3, 5);
        if ((o->timer & 3) == 1)
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_CRITTER_FLY);

        if (o->blockd)
          o->yinertia = -0x200;
      }
    }
    break;

    case 5: // coming down from flight
    {
      if (o->blockd)
      { // landed
      landed:;
        if (o->type == OBJ_CRITTER_HOPPING_BLUE)
        {
          o->damage = 1; // reset to normal damage
        }
        else
        {
          o->damage = 2; // reset to normal damage
        }
        o->state = 1;

        o->frame    = 0;
        o->timer    = 0;
        o->xinertia = 0;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);

        if (o->type == OBJ_POWER_CRITTER)
          quake(30);
      }
    }
    break;
  }

  if (o->state == 4) // flying
  {
    // fly towards player
    o->xinertia += (o->x > player->x) ? -0x20 : 0x20;
    // sine-wave oscillate
    o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;

    LIMITX(0x200);
    LIMITY(0x200);
  }
  else if (o->state == 3 && o->yinertia < 0) // jumping up
  {
    o->yinertia += o->critter.jumpgrav;
  }
  else
  {
    o->yinertia += 0x40;
  }

  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_bat_hang(Object *o)
{
  switch (o->state)
  {
    case 0:
      o->state = 1;
    case 1: // hanging and waiting
      if (!random(0, 100))
      {
        o->state = 2;
        o->timer = 0;
        o->frame = 1;
      }

      if (pdistlx(0x1000) && pdistly2(0x1000, 0x9000))
      {
        o->frame = 0;
        o->state = 3;
      }
      break;

    case 2: // blinking
      if (++o->timer > 8)
      {
        o->state = 1;
        o->frame = 0;
      }
      break;

    case 3: // at attention
      o->frame = 0;
      if (o->shaketime || pdistlx(0x2800))
      {
        o->frame     = 1;
        o->animtimer = 0;
        o->state     = 4;
        o->timer     = 0;
      }
      break;

    case 4: // falling
      o->yinertia += 0x20;
      LIMITY(0x5ff);

      o->timer++;
      if (o->timer > 20 || o->blockd)
      {
        if (o->blockd || ((player->y - 0x2000) < o->y))
        { // start flying
          o->animtimer = 0;
          o->frame     = 2;
          o->state     = 5;
          o->ymark     = o->y;
          if (o->blockd)
            o->yinertia = -0x200;
        }
      }
      break;

    case 5: // flying
      ANIMATE(1, 2, 4);
      FACEPLAYER;

      o->xinertia += (o->x > player->x) ? -0x20 : 0x20;
      o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;

      LIMITX(0x200);
      LIMITY(0x200);

      if (o->blockd)
        o->yinertia = -0x200;
      if (o->blockr)
        o->yinertia = 0x200;
      break;
  }
}

void ai_bat_circle(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      uint8_t angle;
      o->state = 1;

      // set up initial direction and target x,y
      angle       = random(0, 255);
      o->xinertia = sin_table[angle];

      angle += 64;
      o->xmark = (o->x + (sin_table[angle] * 8));

      angle       = random(0, 255);
      o->yinertia = sin_table[angle];

      angle += 64;
      o->ymark = (o->y + (sin_table[angle] * 8));
    }
    case 1:
      // circle around our target point
      ANIMATE(1, 2, 4);
      FACEPLAYER;
      o->xinertia += (o->x > o->xmark) ? -0x10 : 0x10;
      o->yinertia += (o->y > o->ymark) ? -0x10 : 0x10;
      LIMITX(0x200);
      LIMITY(0x200);

      if (!o->timer2)
      {
        if (pdistlx(0x1000) && (player->y > o->y) && pdistly(0xC000))
        { // dive attack
          o->xinertia /= 2;
          o->yinertia = 0;
          o->state    = 2;
          o->frame    = 5; // mouth showing teeth
        }
      }
      else
        o->timer2--;
      break;

    case 2: // dive attack
      o->yinertia += 0x40;
      LIMITY(0x5ff);

      if (o->blockd)
      {
        o->yinertia = 0;
        o->xinertia *= 2;
        o->timer2 = 120; // delay before can dive again
        o->state  = 1;
      }
      break;
  }
}

/*
void c------------------------------() {}
*/

void ai_jelly(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->nxflags |= NXFLAG_SLOW_WHEN_HURT;
      o->timer = random(0, 20);
      o->xmark = o->x;
      o->ymark = o->y;

      o->xinertia = (o->dir == LEFT) ? 0x200 : -0x200;
      o->state    = 1;
    }
    case 1:
    {
      if (--o->timer <= 0)
      {
        o->state = 10;
      }
      else
        break;
    }
    case 10:
    {
      if (++o->timer > 10)
      {
        o->timer = o->frame = 0;
        o->state            = 11;
      }
    }
    break;

    case 11:
    {
      if (++o->animtimer > 5)
      {
        o->animtimer = 0;
        o->frame++;
      }

      if (o->frame == 2)
      {
        o->xinertia += (o->dir == LEFT) ? -0x100 : +0x100;
        o->yinertia -= 0x200;
      }
      else if (o->frame > 2)
      {
        o->state = 12;
        o->frame = 3;
      }
    }
    break;

    case 12:
    {
      o->timer++;
      if (o->y > o->ymark && o->timer > 10)
      {
        o->timer = 0;
        o->state = 10;
        o->frame = 0;
      }
    }
    break;
  }

  o->dir = (o->x > o->xmark) ? LEFT : RIGHT;

  if (o->blockl)
    o->dir = RIGHT;
  if (o->blockr)
    o->dir = LEFT;
  if (o->blockd)
    o->yinertia = -0x200;

  o->yinertia += 0x20;

  LIMITX(0x100);
  LIMITY(0x200);
}

// Kulala
void ai_giant_jelly(Object *o)
{
  switch (o->state)
  {
    case 0: // frozen/in stasis. waiting for player to shoot.
      o->frame = 4;
      if (o->shaketime)
      {
        quake(30);
        o->state = 10;
        o->frame = 0;
        o->timer = 0;
      }
      break;

    case 10: // falling
    {
      o->flags |= FLAG_SHOOTABLE;
      o->flags &= ~FLAG_INVULNERABLE;

      if (++o->timer > 40)
      {
        o->timer     = 0;
        o->animtimer = 0;
        o->state     = 11;
      }
    }
    break;

    case 11: // animate thrust
    {
      ANIMATE_FWD(5);
      if (o->frame >= 3)
      {
        o->frame = 3;
        o->state = 12;
      }
    }
    break;

    case 12: // thrusting upwards
    {
      o->yinertia = -0x155;
      if (++o->timer > 20)
      {
        o->state = 10;
        o->frame = 0;
        o->timer = 0;
      }
    }
    break;

    case 20: // shot/freeze over/go invulnerable
    {
      o->frame = 4;
      o->xinertia >>= 1;
      o->yinertia += 0x20;

      if (!o->shaketime)
      {
        o->state = 10;
        o->frame = 0;
        o->timer = 30;
      }
    }
    break;
  }

  if (o->shaketime)
  {
    if (++o->timer3 > 12)
    {
      o->state = 20;
      o->frame = 4;
      o->flags &= ~FLAG_SHOOTABLE;
      o->flags |= FLAG_INVULNERABLE;
    }
  }
  else
  {
    o->timer3 = 0;
  }

  if (o->state >= 10)
  {
    if (o->blockl)
    {
      o->timer2 = 50;
      o->dir    = RIGHT;
    }
    if (o->blockr)
    {
      o->timer2 = 50;
      o->dir    = LEFT;
    }

    if (o->timer2 > 0)
    {
      o->timer2--;
      XACCEL(0x80);
    }
    else
    {
      o->timer2 = 50;
      FACEPLAYER;
    }

    o->yinertia += 0x10;
    if (o->blockd)
      o->yinertia = -0x300;
  }

  LIMITX(0x100);
  LIMITY(0x300);
}

/*
void c------------------------------() {}
*/

void ai_mannan(Object *o)
{
  // check if we were "killed"
  if (o->state < 3 && o->hp < 90)
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LITTLE_CRASH);
    SmokeClouds(o, 8, 12, 12);
    o->SpawnXP(objprop[o->type].xponkill);
    o->flags &= ~FLAG_SHOOTABLE;
    o->state  = 3;
    o->timer  = 0;
    o->frame  = 2;
    o->damage = 0;
  }

  switch (o->state)
  {
    case 0:
      if (o->shaketime)
      {
        SpawnObjectAtActionPoint(o, OBJ_MANNAN_SHOT);
        o->frame = 1;
        o->state = 2;
        o->timer = 0;
      }
      break;

    case 2: // firing
      if (++o->timer > 20)
      {
        o->timer = o->state = o->frame = 0;
      }
      break;

    case 3: // dead/blinking
      switch (++o->timer)
      {
        case 50:
        case 60:
          o->frame = 3;
          break;
        case 53:
        case 63:
          o->frame = 2;
          break;
        case 100:
          o->state = 4;
          break;
      }
      break;
  }
}

void ai_mannan_shot(Object *o)
{
  XACCEL(0x20);
  ANIMATE(0, 1, 2);

  if ((o->timer & 3) == 1)
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_IRONH_SHOT_FLY);

  if (++o->timer > 100)
    o->Delete();
}

/*
void c------------------------------() {}
*/

// there is an apparent bug on the sprite sheet for this monster:
//	right-facing frame 1 is a duplicate of frame 0,
//	so the mouth-twitch animation does not work when
//	the frog is facing right.
void ai_frog(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      o->timer    = 0;
      o->xinertia = 0;
      o->yinertia = 0;

      // non-normal dirs are used to indicate that this frog was
      // spawned by balfrog: we are falling out of ceiling during fight
      if (o->dir != LEFT && o->dir != RIGHT)
      {
        o->dir = random(0, 1) ? LEFT : RIGHT;
        o->flags |= FLAG_IGNORE_SOLID;

        o->state = 3;
        o->frame = 2;
      }
      else
      {
        o->flags &= ~FLAG_IGNORE_SOLID;
        o->state = 1;
      }
    }
    case 1: // standing
    {
      o->timer++;

      if (!random(0, 50))
      { // mouth-flitter animation
        o->state     = 2;
        o->frame     = 0;
        o->timer     = 0;
        o->animtimer = 0;
      }
    }
    break;

    case 2: // mouth flitter
    {
      ANIMATE(2, 0, 1);

      if (++o->timer > 18)
        o->state = 1;
    }
    break;

    case 3: // falling out of ceiling during balfrog fight
    {
      if (++o->timer > 40)
      {
        o->flags &= ~FLAG_IGNORE_SOLID;

        if (o->blockd)
        {
          o->state = 0;
          o->frame = 0;
          o->timer = 0;
        }
      }
    }
    break;

    case 10: // jumping
    case 11:
    {
      if (o->blockl && o->xinertia < 0)
      {
        o->dir      = RIGHT;
        o->xinertia = -o->xinertia;
      }

      if (o->blockr && o->xinertia > 0)
      {
        o->dir      = LEFT;
        o->xinertia = -o->xinertia;
      }

      if (o->blockd)
      {
        o->state = 0;
        o->frame = 0;
        o->timer = 0;
      }
    }
    break;
  }

  // random jumping, and jump when shot
  if (o->state < 3 && o->timer > 10)
  {
    bool dojump = false;

    if (o->shaketime)
    {
      dojump = true;
    }
    else if (pdistlx(0x14000) && pdistly(0x8000))
    {
      if (!random(0, 50))
      {
        dojump = true;
      }
    }

    if (dojump)
    {
      FACEPLAYER;
      o->state    = 10;
      o->frame    = 2;
      o->yinertia = -0x5ff;

      // no jumping sound in cutscenes at ending
      if (!player->inputs_locked && !player->disabled)
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_JUMP);

      XMOVE(0x200);
    }
  }

  o->yinertia += 0x80;
  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

void ai_hey_spawner(Object *o)
{
  if (!o->state)
  {
    effect(o->x, o->y - (8 * CSFI), EFFECT_HEY);
    o->state = 1;
  }
}

void ai_motorbike(Object *o)
{

  switch (o->state)
  {
    case 0: // parked
      break;

    case 10: // kazuma and booster mounted
      o->y -= (sprites[SPR_MOTORBIKE_MOUNTED].h - sprites[SPR_MOTORBIKE].h) * CSFI;
      o->sprite = SPR_MOTORBIKE_MOUNTED;
      o->state++;
      break;

    case 20: // kazuma and booster start the engine
    {
      o->state = 21;
      o->timer = 1;

      o->xmark = o->x;
      o->ymark = o->y;
    }
    case 21:
    {
      o->x = o->xmark + (random(-1, 1) * CSFI);
      o->y = o->ymark + (random(-1, 1) * CSFI);

      if (++o->timer > 30)
        o->state = 30;
    }
    break;

    case 30: // kazuma and booster take off
    {
      o->state = 31;
      o->timer = 1;

      o->xinertia = -0x800;
      o->ymark    = o->y;

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MISSILE_HIT);
    }
    case 31:
    {
      o->xinertia += 0x20;
      o->timer++;

      o->y = o->ymark + (random(-1, 1) * CSFI);

      if (o->timer > 10)
        o->dir = RIGHT;
      if (o->timer > 200)
        o->state = 40;
    }
    break;

    case 40: // flying away (fast out-of-control)
    {
      o->state = 41;
      o->timer = 2;
      o->dir   = LEFT;
      o->y -= (48 * CSFI);   // move up...
      o->xinertia = -0x1000; // ...and fly fast
    }
    case 41:
    {
      o->timer += 2; // makes exhaust sound go faster
      if (o->timer > 1200)
        o->Delete();
    }
    break;
  }

  if (o->state >= 20 && (o->timer & 3) == 0)
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FIREBALL);

    // make exhaust puffs, and make them go out horizontal
    // instead of straight up as this effect usually does
    Caret *puff    = effect(o->ActionPointX(), o->ActionPointY(), EFFECT_SMOKETRAIL_SLOW);
    puff->yinertia = 0;
    puff->xinertia = (o->dir == LEFT) ? 0x280 : -0x280;
  }
}

/*
void c------------------------------() {}
*/

void ai_malco(Object *o)
{

  switch (o->state)
  {
    case 0:
      o->state = 1;
      o->frame = 0;

      // smushed into ground, used on re-entry to room
      // if player does not choose to pull him out
      // after Balrog fight
      if (o->dir == RIGHT)
        o->frame = 5;
      break;

    case 10:
      o->state = 11;
      o->timer = 0;
      o->timer = 0;
      SmokeClouds(o, 4, 16, 16);
    case 11: // beeping and eyes flickering
      if (++o->timer < 100)
      {
        if ((o->timer & 1) == 0)
        {
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
          o->frame ^= 1;
        }
      }
      else if (o->timer > 150)
      {
        o->timer = 0;
        o->state = 15;
      }
      break;

    case 15: // shaking
      if (o->timer & 2)
      {
        o->x += (1 * CSFI);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DOOR);
      }
      else
      {
        o->x -= (1 * CSFI);
      }
      if (++o->timer > 50)
        o->state = 16;
      break;

    case 16: // stand up
      o->state = 17;
      o->frame = 2;
      o->timer = 0;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
      SmokeClouds(o, 4, 16, 16);
    case 17:
      if (++o->timer > 150)
      {
        o->state = 18;
      }
      break;

    case 18: // gawking/bobbing up and down
      o->state = 19;
      o->timer = 0;
      // go into gawk frame first time
      o->animtimer = 999;
      o->frame     = 9;
    case 19:
      if (++o->animtimer > 3)
      {
        o->animtimer = 0;
        if (++o->frame > 4)
          o->frame = 3;
        if (o->frame == 3)
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DOOR);
      }
      if (++o->timer > 100)
      {
        o->state = 20;
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DOOR);
        SmokeClouds(o, 4, 16, 16);
      }
      break;

    case 20:
      o->frame = 4;
      break;

    case 21: // got smushed!
      o->state = 22;
      o->frame = 5;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_ENEMY_HURT);
      break;

    case 22:
      break;

    case 100: // "baby" malcos waking up during credits
    {
      o->state     = 101;
      o->frame     = 3;
      o->animtimer = 0;
    }
    case 101:
    {
      ANIMATE(4, 3, 4);
    }
    break;
    case 110: // the one that blows up
    {
      SmokeClouds(o, 16, 4, 4);
      o->Delete();
    }
    break;
  }
}

void ai_malco_broken(Object *o)
{
  switch (o->state)
  {
    case 10: // set when pulled out of ground
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
      SmokeClouds(o, 4, 16, 16);
      o->state = 0;
      break;

    case 0:
    {
      o->frame = 0;
      randblink(o, 1, 8, 50);

      if (game.mode != GM_CREDITS)
        FACEPLAYER;
    }
    break;
  }
}
