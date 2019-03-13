#include "balrog.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/Tileset.h"
#include "../../map.h"
#include "../../sound/SoundManager.h"
#include "../../tsc.h"
#include "../ai.h"
#include "../stdai.h"
#include "../sym/smoke.h"

INITFUNC(AIRoutines)
{
  ONSPAWN(OBJ_BALROG, onspawn_balrog);
  ONTICK(OBJ_BALROG, ai_balrog);

  ONTICK(OBJ_BALROG_DROP_IN, ai_balrog_drop_in);
  ONTICK(OBJ_BALROG_BUST_IN, ai_balrog_bust_in);
}

/*
void c------------------------------() {}
*/

void onspawn_balrog(Object *o)
{
  // in the Boulder Chamber boss fight, Balrog is supposed to go BEHIND Curly.
  if (game.curmap == STAGE_BOULDER_CHAMBER)
  {
    Object *curly = Objects::FindByType(OBJ_CURLY);
    if (curly)
    {
      o->PushBehind(curly);

      // nasty hack to adjust his starting position in the post-fight cutscene.
      // I'm not sure why this is otherwise wrong.
      if (game.tsc->GetCurrentScript() == 600)
      {
        o->x -= (6 * CSFI);
      }
    }
  }
}

void ai_balrog(Object *o)
{
  bool fall = true;

  // he is greenish when he first appears in Gum Room
  if (DoesCurrentStageUseSpriteset(NPCSET_FROG))
    o->sprite = SPR_BALROG_GREEN;

  switch (o->state)
  {
    case 0:
    {
      o->flags &= ~FLAG_IGNORE_SOLID;
      o->xinertia       = 0;
      o->balrog.smoking = false;

      o->frame = 0;
      randblink(o, 4, 8);
    }
    break;

    case 10: // he jumps and flys away
      o->xinertia = 0;
      o->frame    = 2;
      o->timer    = 0;
      o->state++;
    case 11:
    {
      if (++o->timer <= 20)
        break;

      o->state++;
      o->yinertia = -0x800;
      o->flags |= FLAG_IGNORE_SOLID;
    }
    case 12:
    {
      fall     = false;
      o->frame = 3;
      o->yinertia -= 0x10;
      if (o->y < 0)
      {
        o->Delete();
        quake(30);
        //				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_QUAKE);
        //				game.quaketime = 30;
      }
    }
    break;

    // he looks shocked and shakes, then flys away
    // used when he is "hit by something"
    case 20:
    {
      o->state    = 21;
      o->frame    = 5;
      o->xinertia = 0;
      o->timer = o->timer2 = 0;
      SmokeClouds(o, 4, 8, 8);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BIG_CRASH);
      o->balrog.smoking = 1;
    }
    case 21:
    {
      o->timer2++;
      o->x += ((o->timer2 >> 1) & 1) ? (1 * CSFI) : -(1 * CSFI);

      if (++o->timer > 100)
        o->state = 10;

      o->yinertia += 0x20;
      LIMITY(0x5ff);
    }
    break;

    case 30: // he smiles for a moment
      o->frame = 6;
      o->timer = 0;
      o->state = 31;
    case 31:
      if (++o->timer > 100)
        o->state = o->frame = 0;
      break;

    // flashing white (spell casted on him)
    // this only works in Gum Room before balfrog fight, as the normal
    // non-greenish spritesheet doesn't include the required frame.
    case 40:
      o->state     = 41;
      o->animtimer = 0;
      o->animframe = 0;
    case 41:
    {
      static const int flash_seq[] = {5, 7};
      o->animate_seq(1, flash_seq, 2);
    }
    break;
    case 42:
      o->timer = 0;
      o->state = 43;
    case 43:
      // flashing visibility
      // (transforming into Balfrog stage boss;
      //	our flashing is interlaced with his)
      o->timer++;
      o->invisible = (o->timer & 2) ? false : true;
      break;

    case 50: // he faces away
      o->frame    = 8;
      o->xinertia = 0;
      break;

    case 60: // he walks
      o->state = 61;
      balrog_walk_init(o);
    case 61:
    {
      balrog_walk_animation(o);
      XMOVE(0x200);
    }
    break;

    // he is teleported away (looking distressed)
    // this is when he is sent to Labyrinth at end of Sand Zone
    case 70:
      o->xinertia = 0;
      o->timer    = 0;
      o->frame    = 7;
      o->state++;
    case 71:
      if (DoTeleportOut(o, 2))
        o->Delete();
      break;

    case 80: // hands up and shakes
      o->frame = 5;
      o->state = 81;
    case 81:
    {
      if (++o->timer & 2)
        o->x += (1 * CSFI);
      else
        o->x -= (1 * CSFI);
    }
    break;

    // fly up and lift Curly & PNPC
    // (post-Ballos ending scene)
    case 100:
    {
      o->state = 101;
      o->timer = 0;
      o->frame = 2; // prepare for jump
    }
    case 101:
    {
      if (++o->timer > 20)
      {
        o->state = 102;
        o->timer = 0;
        o->frame = 3; // fly up

        DeleteObjectsOfType(OBJ_NPC_PLAYER);
        DeleteObjectsOfType(OBJ_CURLY);

        CreateObject(0, 0, OBJ_BALROG_PASSENGER, 0, 0, LEFT)->linkedobject  = o;
        CreateObject(0, 0, OBJ_BALROG_PASSENGER, 0, 0, RIGHT)->linkedobject = o;

        o->yinertia = -0x800;
        o->flags |= FLAG_IGNORE_SOLID; // so can fly through ceiling
        fall = false;
      }
    }
    break;
    case 102: // flying up during escape seq
    {
      fall = false;

      // bust through ceiling
      int y = ((o->y + (4 * CSFI)) / CSFI) / TILE_H;
      if (y < 35 && y >= 0)
      {
        int x = (o->CenterX() / CSFI) / TILE_W;

        if (map.tiles[x][y] != 0)
        {
          // smoke needs to go at the bottom of z-order or you can't
          // see any of the characters through all the smoke.
          map_ChangeTileWithSmoke(x, y, 0, 4, false, lowestobject);
          map_ChangeTileWithSmoke(x - 1, y, 0, 4, false, lowestobject);
          map_ChangeTileWithSmoke(x + 1, y, 0, 4, false, lowestobject);

          megaquake(10, NXE::Sound::SFX::SND_NULL);
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MISSILE_HIT);
        }
      }

      if (o->Bottom() < -(20 * CSFI))
      {
        quake(30, NXE::Sound::SFX::SND_NULL);
        o->Delete();
      }
    }
    break;

    case 500: // used during Balfrog death scene
    {
      fall = false;
    }
    break;
  }

  if (o->balrog.smoking)
  {
    if (++o->balrog.smoketimer > 20 || !random(0, 16))
    {
      SmokeClouds(o, 1, 4, 4);
      o->balrog.smoketimer = 0;
    }
  }

  if (fall)
  {
    o->yinertia += 0x20;
    if (o->yinertia >= 0x5ff)
      o->yinertia = 0x5ff;
  }
}

void ai_balrog_drop_in(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      // he is greenish when he first appears in Gum Room
      if (DoesCurrentStageUseSpriteset(NPCSET_FROG))
        o->sprite = SPR_BALROG_GREEN;

      // z-order hacking
      if (game.curmap == STAGE_SEAL_CHAMBER_2)
        o->PushBehind(lowestobject);

      o->state = 1;
      o->frame = 3; // falling
      o->flags |= FLAG_IGNORE_SOLID;
    }
    case 1:
    {
      // since balrog often falls through the ceiling we must wait until he is free-falling
      // before we start checking to see if he hit the floor
      if (!o->blockd && !o->blocku)
      {
        o->state = 2;
        o->flags &= ~FLAG_IGNORE_SOLID;
      }
    }
    break;

    case 2: // free-falling
      if (o->blockd)
      {
        o->yinertia = 0;
        o->frame    = 2;
        o->state    = 3;
        o->timer    = 0;

        SmokeSide(o, 4, DOWN);
        quake(30);
      }
      break;

    case 3: // landed
      if (++o->timer > 20)
      {
        o->state = 4;
        o->frame = 0;
      }
      break;
  }

  if (o->state == 1 || o->state == 2)
    o->yinertia += 0x20;
}

// Balrog busting in the door of the Shack.
// he exists like this for only a moment, then the script
// changes him to a standard OBJ_BALROG.
void ai_balrog_bust_in(Object *o)
{
  switch (o->state)
  {
    case 0:
      SmokeClouds(o, 10, 8, 8);
      o->y += (10 * CSFI);
      o->yinertia = -0x100;

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
      quake(30);

      o->state = 1;
      o->frame = 3;
    case 1: // falling the short distance to ground
    {
      o->yinertia += 0x10;
      if (o->yinertia > 0 && o->blockd)
      {
        o->state = 2;
        o->frame = 2;
        o->timer = 0;

        quake(30);
      }
    }
    break;

    // landing animation
    case 2:
    {
      if (++o->timer > 16)
      {
        o->state     = 3;
        o->frame     = 0;
        o->animtimer = 0;
      }
    }
    break;

    // standing and blinking
    case 3:
    case 4:
    {
      o->frame = 0;
      randblink(o, 4, 16, 100);
    }
    break;
  }

  LIMITY(0x5FF);
}

/*
void c------------------------------() {}
*/

void balrog_walk_init(Object *o)
{
  o->frame     = 9;
  o->animtimer = 0;
}

void balrog_walk_animation(Object *o)
{
  if (++o->animtimer > 3)
  {
    o->animtimer = 0;
    o->frame++;

    if (o->frame == 10 || o->frame == 11)
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);

    if (o->frame > 12)
      o->frame = 9;
  }
}
