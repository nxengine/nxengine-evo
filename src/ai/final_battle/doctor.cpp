#include "doctor.h"

#include "../../ObjManager.h"
#include "../../autogen/sprites.h"
#include "../../common/misc.h"
#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../graphics/Tileset.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../../trig.h"
#include "../ai.h"
#include "../stdai.h"
#include "doctor_common.h"

/*
        From King's Table, here's the Doctor's first form.
        He teleports around the room firing red wave shots at you with
        the Red Crystal following him. After every fourth teleport, he
        substitutes the wave attack for a aerial "explosion" of bouncy
        red shots (OBJ_DOCTOR_BLAST).
*/

INITFUNC(AIRoutines)
{
  ONTICK(OBJ_BOSS_DOCTOR, ai_boss_doctor);
  AFTERMOVE(OBJ_RED_CRYSTAL, aftermove_red_crystal);

  ONTICK(OBJ_DOCTOR_SHOT, ai_doctor_shot);
  ONTICK(OBJ_DOCTOR_SHOT_TRAIL, ai_doctor_shot_trail);

  ONTICK(OBJ_DOCTOR_BLAST, ai_doctor_blast);

  ONTICK(OBJ_DOCTOR_CROWNED, ai_doctor_crowned);
}

/*
void c------------------------------() {}
*/

void ai_boss_doctor(Object *o)
{
  // AIDEBUG;

  /*if (o->state > 2 && o->state < 500)
  {
          o->state = 937;
          game.tsc->StartScript(410);
          return;
  }*/

  switch (o->state)
  {
    case 0:
    {
      o->y += (8 * CSFI);
      o->frame = 3;
      o->state = 1;
      o->BringToFront();      // make sure in front of doctor_crowned
      crystal_tofront = true; // make sure front crystal is in front of us
    }
    break;

    case 2: // transforming (script)
    {
      o->timer++;
      o->frame = (o->timer & 2) ? 0 : 3;

      if (o->timer > 50)
        o->state = 10;
    }
    break;

    case 10: // base state/falling (script)
    {
      o->yinertia += 0x80;
      o->flags |= FLAG_SHOOTABLE;
      o->damage = 3;

      if (o->blockd)
      {
        o->state = 20;
        o->timer = 0;
        o->frame = 0;

        o->savedhp = o->hp;
        FACEPLAYER;
      }
    }
    break;

    // fire wave shot
    case 20:
    {
      o->timer++;

      if (o->timer < 50)
      {
        if ((o->hp - o->savedhp) > 20)
          o->timer = 50;
      }

      if (o->timer == 50)
      { // arm across chest
        FACEPLAYER;
        o->frame = 4;
      }

      if (o->timer == 80)
      {
        Object *shot;
        o->frame = 5; // arm cast out

        shot        = SpawnObjectAtActionPoint(o, OBJ_DOCTOR_SHOT);
        shot->dir   = o->dir;
        shot->angle = 0;

        shot        = SpawnObjectAtActionPoint(o, OBJ_DOCTOR_SHOT);
        shot->dir   = o->dir;
        shot->angle = 0x80;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_FUNNY_EXPLODE);
      }

      if (o->timer == 120)
        o->frame = 0; // arm down

      if (o->timer > 130)
      {
        if ((o->hp - o->savedhp) > 50)
        {
          o->state = 100;
          o->timer = 0;
        }

        if (o->timer > 160)
        {
          o->state = 100;
          o->timer = 0;
        }
      }
    }
    break;

    // big "explosion" blast
    case 30:
    {
      o->state = 31;
      o->timer = 0;
      o->frame = 6;
      o->xmark = o->x;
      o->flags |= FLAG_SHOOTABLE;
    }
    case 31:
    {
      o->x = o->xmark;
      if (++o->timer & 2)
        o->x += (1 * CSFI);

      if (o->timer > 50)
      {
        o->state = 32;
        o->timer = 0;
        o->frame = 7;

        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LIGHTNING_STRIKE);

        for (int angle = 8; angle < 256; angle += 16)
        {
          Object *shot = SpawnObjectAtActionPoint(o, OBJ_DOCTOR_BLAST);
          ThrowObjectAtAngle(shot, angle, 0x400);
        }
      }
    }
    break;
    case 32: // after blast
    {
      if (++o->timer > 50)
        o->state = 100;
    }
    break;

    // teleport away
    case 100:
    {
      o->state = 101;
      o->flags &= ~FLAG_SHOOTABLE;
      o->damage = 0;

      dr_tp_out_init(o);
    }
    case 101:
    {
      if (dr_tp_out(o))
      {
        o->state     = 102;
        o->timer     = 0;
        o->invisible = true;

        // decide where we're going to go now, so the red crystal
        // can start moving towards it. But, it's important not to
        // actually move until the last possible second, or we could
        // drag our floattext along with us (and give away our position).
        o->xmark = (random(5, 35) * TILE_W) * CSFI;
        o->ymark = (random(5, 7) * TILE_H) * CSFI;
      }
    }
    break;

    case 102: // invisible: waiting to reappear
    {
      if (++o->timer > 40)
      {
        o->state    = 103;
        o->timer    = 16;
        o->frame    = 2;
        o->yinertia = 0;

        o->x = o->xmark;
        o->y = o->ymark;

        FACEPLAYER;
      }
    }
    break;

    // tp back in
    case 103:
    {
      o->state++;
      dr_tp_in_init(o);
    }
    case 104:
    {
      if (dr_tp_in(o))
      {
        o->flags |= FLAG_SHOOTABLE;
        o->damage = 3;

        if (++o->timer2 >= 4)
        { // big explode
          o->timer2 = 0;
          o->state  = 30;
        }
        else
        { // another wave shot
          o->state = 10;
        }
      }
    }
    break;

    // defeated!
    case 500:
    {
      o->flags &= ~FLAG_SHOOTABLE;
      o->frame = 6;

      // fall to earth
      o->yinertia += 0x10;
      if (o->blockd && o->yinertia >= 0)
      {
        o->state = 501;
        o->timer = 0;

        o->xmark = o->x;
        FACEPLAYER;
      }
    }
    break;

    case 501: // flashing (transforming into Doctor 2)
    {
      FACEPLAYER;
      o->frame = 8;

      o->x = o->xmark;
      if (!(++o->timer & 2))
        o->x += (1 * CSFI);
    }
    break;
  }

  // enable per-frame bbox
  COPY_PFBOX;

  // set crystal follow position
  if (o->state >= 10)
  {
    if (o->invisible) // teleporting
    {
      crystal_xmark = o->xmark;
      crystal_ymark = o->ymark;
    }
    else
    {
      crystal_xmark = o->x;
      crystal_ymark = o->y;
    }
  }

  LIMITY(0x5ff);
}

/*
void c------------------------------() {}
*/

// wave shot
void ai_doctor_shot(Object *o)
{
  if (o->x < 0 || o->x > ((map.xsize * TILE_W) * CSFI))
  {
    o->Delete();
    return;
  }

  switch (o->state)
  {
    case 0:
    {
      o->state = 1;
      o->xmark = o->x;
      o->ymark = o->y;
    }
    case 1:
    {
      // distance apart from each other
      if (o->timer2 < 128)
        o->timer2++;

      // spin
      o->angle += 6;

      // travel
      o->speed += (o->dir == LEFT) ? -0x15 : 0x15;
      o->xmark += o->speed;

      o->x = o->xmark + (xinertia_from_angle(o->angle, o->timer2 * CSFI) / 8);
      o->y = o->ymark + (yinertia_from_angle(o->angle, o->timer2 * CSFI) / 2);

      Object *trail = CreateObject(o->x, o->y, OBJ_DOCTOR_SHOT_TRAIL);
      trail->sprite = SPR_DOCTOR_SHOT;
      trail->frame  = 1;
      trail->PushBehind(o);
    }
    break;
  }
}

void ai_doctor_shot_trail(Object *o)
{
  ANIMATE_FWD(3);
  if (o->frame > 3)
    o->Delete();
}

// from his "explosion" attack
void ai_doctor_blast(Object *o)
{
  // they're bouncy
  if ((o->blockl && o->xinertia < 0) || (o->blockr && o->xinertia > 0))
  {
    o->xinertia = -o->xinertia;
  }

  if (o->blockd && o->yinertia > 0)
    o->yinertia = -0x200;

  if (o->blocku && o->yinertia < 0)
    o->yinertia = 0x200;

  ANIMATE(0, 0, 1);

  if ((++o->timer % 4) == 1)
    CreateObject(o->x, o->y, OBJ_DOCTOR_SHOT_TRAIL)->PushBehind(o);

  if (o->timer > 250)
    o->Delete();
}

/*
void c------------------------------() {}
*/

// The Doctor's red crystal.
// There are actually two, one is behind him and one is in front
// and they alternate visibility as they spin around him so it looks 3D.
//
// This function has to be an aftermove, otherwise, because one is in front
// and the other behind, one will be checking crystal_xmark before the Doctor
// updates it, and the other afterwards, and they will get out of sync.
void aftermove_red_crystal(Object *o)
{
  ANIMATE(3, 0, 1);

  switch (o->state)
  {
    case 0:
    {
      if (crystal_xmark != 0)
      {
        o->state        = 1;
        crystal_tofront = true;
      }
    }
    break;

    case 1:
    {
      o->xinertia += (o->x < crystal_xmark) ? 0x55 : -0x55;
      o->yinertia += (o->y < crystal_ymark) ? 0x55 : -0x55;
      LIMITX(0x400);
      LIMITY(0x400);

      if ((o->dir == LEFT && o->xinertia > 0) || (o->dir == RIGHT && o->xinertia < 0))
      {
        o->invisible = true;
      }
      else
      {
        o->invisible = false;
      }
    }
    break;
  }

  if (crystal_tofront && o->dir == LEFT)
  {
    o->BringToFront();
    crystal_tofront = false;
  }
}

/*
void c------------------------------() {}
*/

// doctor as npc before fight
void ai_doctor_crowned(Object *o)
{
  switch (o->state)
  {
    case 0:
    {
      // do this manually instead of a spawn point,
      // cause he's gonna transform.
      o->x -= (8 * CSFI);
      o->y -= (16 * CSFI);

      o->state      = 1;
      crystal_xmark = crystal_ymark = 0;
      crystal_tofront               = true;
    }
    case 1: // faces away
    {
      o->frame = 0;
    }
    break;

    case 10: // goes "ho ho ho" (while still facing away)
    {
      o->frame     = 0;
      o->animtimer = 0;
      o->timer     = 0;
      o->state     = 11;
    }
    case 11:
    {
      ANIMATE(5, 0, 1);
      // he has to show shrug frame exactly 6 times.
      // ANIMATE(5) changes frame on every 6th tick
      // so this is 6*6*nframes(2) = 72
      if (++o->timer >= 72)
        o->state = 1;
    }
    break;

    case 20: // turns around (faces screen instead of away)
    {
      o->state = 21;
      o->frame = 2;
    }
    break;

    case 40: // arm up--presents red crystal
    {
      o->state = 41;

      // spawn the red crystal
      // one is for when it's behind him, the other is in front.
      int x = o->x - (6 * CSFI);
      int y = o->y - (8 * CSFI);

      dr_create_red_crystal(x, y);
    }
    case 41:
    {
      o->frame = 4;
    }
    break;

    case 50: // "ho ho ho" (while facing player)
    {
      o->frame     = 4;
      o->animtimer = 0;
      o->timer     = 0;
      o->state     = 51;
    }
    case 51:
    {
      ANIMATE(5, 4, 5);
      if (++o->timer >= 72)
        o->state = 41;
    }
    break;
  }
}
