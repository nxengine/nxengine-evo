
#include "ai.h"

#include "../ObjManager.h"
#include "../ResourceManager.h"
#include "../caret.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../player.h"
#include "../sound/SoundManager.h"
#include "stdai.h"
#include "sym/smoke.h"

using namespace NXE::Graphics;

InitList AIRoutines;

bool ai_init(void)
{
  // setup function pointers to AI routines
  for (int i = 0; i < OBJ_LAST; i++)
    memset(&objprop[i].ai_routines, 0, sizeof(objprop[i].ai_routines));

  if (load_npc_tbl())
    return 1;

  // OBJ_NULL has flags set in npc.tbl, but shouldn't be set in our engine
  objprop[OBJ_NULL].defaultflags = 0;
  memcpy(&objprop[OBJ_SKULLHEAD_CARRIED], &objprop[OBJ_SKULLHEAD], sizeof(ObjProp));

  objprop[OBJ_POLISH].initial_hp  = 24; // is the value of 120 in npc.tbl really wrong? if so why?
  objprop[OBJ_POLISH].death_sound = NXE::Sound::SFX::SND_FUNNY_EXPLODE; // not sure why this is apparently wrong in file

  // call all the INITFUNC() routines you find at the beginning
  // of every AI-related module which assign AI logic to objects.
  if (AIRoutines.CallFunctions())
  {
    LOG_ERROR("ai_init: failed to initialize AIRoutines function pointers");
    return 1;
  }

  return 0;
}

bool load_npc_tbl(void)
{
  const int smoke_amounts[] = {0, 3, 7, 12};
  const int nEntries        = 361;
  int i;
  FILE *fp = myfopen(widen(ResourceManager::getInstance()->getPath("npc.tbl")).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("load_npc_tbl: data/npc.tbl is missing");
    return 1;
  }

  LOG_INFO("Reading npc.tbl...");

  for (i = 0; i < nEntries; i++)
    objprop[i].defaultflags = fgeti(fp);
  for (i = 0; i < nEntries; i++)
    objprop[i].initial_hp = fgeti(fp);

  // next is a spritesheet # of something--but we don't use it, so skip
  // for(i=0;i<nEntries;i++) fgetc(fp);		// spritesheet # or something--but we don't use it
  fseek(fp, (nEntries * 2 * 2) + nEntries, SEEK_SET);

  for (i = 0; i < nEntries; i++)
    objprop[i].death_sound = (NXE::Sound::SFX)fgetc(fp);
  for (i = 0; i < nEntries; i++)
    objprop[i].hurt_sound = (NXE::Sound::SFX)fgetc(fp);
  for (i = 0; i < nEntries; i++)
    objprop[i].death_smoke_amt = smoke_amounts[fgetc(fp)];
  for (i = 0; i < nEntries; i++)
    objprop[i].xponkill = fgetl(fp);
  for (i = 0; i < nEntries; i++)
    objprop[i].damage = fgetl(fp);

  /*for(i=0;i<nEntries;i++)
  {
          int left = fgetc(fp);
          int top = fgetc(fp);
          int right = fgetc(fp);
          int bottom = fgetc(fp);

          if (i == 59)
          {
                  stat("%d %d %d %d", left, top, right, bottom);
                  stat("sprite %d", objprop[i].sprite);
          }
  }*/

  fclose(fp);
  return 0; // 1;
}

/*
void c------------------------------() {}
*/

// spawn an object at an enemies action point
Object *SpawnObjectAtActionPoint(Object *o, int otype)
{
  int x, y;
  Object *newObject;

  x              = o->x + (Renderer::getInstance()->sprites.sprites[o->sprite].frame[o->frame].dir[o->dir].actionpoint.x * CSFI);
  y              = o->y + (Renderer::getInstance()->sprites.sprites[o->sprite].frame[o->frame].dir[o->dir].actionpoint.y * CSFI);
  newObject      = CreateObject(x, y, otype);
  newObject->dir = o->dir;
  return newObject;
}

// destroys all objects of type "otype".
// creates a BoomFlash and smoke, but no bonuses.
void KillObjectsOfType(int type)
{
  Object *o = firstobject;
  while (o)
  {
    if (o->type == type)
    {
      SmokeClouds(o, 1, 0, 0);
      effect(o->CenterX(), o->CenterY(), EFFECT_BOOMFLASH);

      o->Delete();
    }

    o = o->next;
  }
}

// deletes all objects of type "otype" silently, without any smoke or other effects.
void DeleteObjectsOfType(int type)
{
  Object *o = firstobject;
  while (o)
  {
    if (o->type == type)
    {
      o->Delete();
    }

    o = o->next;
  }
}

/*
void c------------------------------() {}
*/

// handles object blinking: at random intervals forces object o's frame to blinkframe
// for blinktime frames.
void randblink(Object *o, int blinkframe, int blinktime, int prob)
{
  if (o->blinktimer)
  {
    o->blinktimer--;
    o->frame = blinkframe;
  }
  else if (random(0, prob) == 0)
  {
    o->frame      = blinkframe;
    o->blinktimer = 8;
  }
}

// call this in an object's aftermove routine if it's an object
// which is being carried by the player like a puppy or curly.
// x_left: offset from p's action point when he faces left
// x_right: when he faces right
// off_y: vertical offset from p's action point
void StickToPlayer(Object *o, int x_left, int x_right, int off_y)
{
  int x, y, frame;

  // needed for puppy in chest
  o->flags &= ~FLAG_SCRIPTONACTIVATE;

  // by offsetting from the player's action point, where he holds his gun, we
  // already have set up for us a nice up-and-down 1 pixel as he walks
  frame = player->frame;
  // the p's "up" frames have unusually placed action points so we have to cancel those out
  if (frame >= 3 && frame <= 5)
    frame -= 3;

  x = (player->x / CSFI) + Renderer::getInstance()->sprites.sprites[player->sprite].frame[frame].dir[player->dir].actionpoint.x;
  y = (player->y / CSFI) + Renderer::getInstance()->sprites.sprites[player->sprite].frame[frame].dir[player->dir].actionpoint.y;
  y += off_y;

  if (player->dir == RIGHT)
  {
    x += x_right;
    o->dir = RIGHT;
  }
  else
  {
    x += x_left;
    o->dir = LEFT;
  }

  o->x = (x * CSFI);
  o->y = (y * CSFI);
}

// used for some bosses with subobjects
void transfer_damage(Object *o, Object *target)
{
  if (o->hp < 1000)
  {
// if you forget to set hp to 1000 when creating the puppet object,
// it can immediately destroy the main object, possibly leading to crashes.
#ifdef DEBUG
    ASSERT(o->hp != 0);
#endif

    target->DealDamage(1000 - o->hp);
    o->hp = 1000;
  }
}

/*
void c------------------------------() {}
*/

// common code for DoTeleportIn and DoTeleportOut
// returns true when teleport is complete
static bool teleffect(Object *o, int slowness, bool teleporting_out)
{
  o->display_xoff = random(-1, 1);

  if (!o->timer)
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TELEPORT);
    o->clip_enable = true;
    o->clipy1      = 0;
  }

  if (++o->timer >= (Renderer::getInstance()->sprites.sprites[o->sprite].h << slowness))
  {
    o->clip_enable  = false;
    o->display_xoff = 0;
    return true;
  }
  else
  {
    int amt = (o->timer >> slowness);

    if (teleporting_out)
      o->clipy2 = Renderer::getInstance()->sprites.sprites[o->sprite].h - amt;
    else
      o->clipy2 = amt;

    return false;
  }
}

// do the "teleport in" effect for object o.
// when complete, returns true.
// this function uses o->timer and assume o->timer starts at 0.
bool DoTeleportIn(Object *o, int slowness)
{
  if (teleffect(o, slowness, false))
  {
    o->clip_enable = false;
    return true;
  }

  return false;
}

// does a teleport out effect.
// When complete, returns true.
// this function uses o->timer and assume o->timer starts at 0.
bool DoTeleportOut(Object *o, int slowness)
{
  return teleffect(o, slowness, true);
}

/*
void c------------------------------() {}
*/

static void simpleanim(Object *o, int spd)
{
  if (++o->animtimer >= spd)
  {
    o->animtimer = 0;
    if (++o->frame >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
      o->frame = 0;
  }
}

void ai_animate1(Object *o)
{
  if (++o->frame >= Renderer::getInstance()->sprites.sprites[o->sprite].nframes)
    o->frame = 0;
}
void ai_animate2(Object *o)
{
  simpleanim(o, 2);
}
void ai_animate3(Object *o)
{
  simpleanim(o, 3);
}
void ai_animate4(Object *o)
{
  simpleanim(o, 4);
}
void ai_animate5(Object *o)
{
  simpleanim(o, 5);
}

void ai_animaten(Object *o, int n)
{
  simpleanim(o, n);
}

/*
void c------------------------------() {}
*/

// aftermove routine which sticks the object to the action point of the NPC that's carrying it
void aftermove_StickToLinkedActionPoint(Object *o)
{
  Object *link = o->linkedobject;
  int dir;

  if (link)
  {
    dir = (link->dir ^ o->carry.flip);

    o->x   = ((link->x / CSFI) + Renderer::getInstance()->sprites.sprites[link->sprite].frame[link->frame].dir[dir].actionpoint.x) * CSFI;
    o->y   = ((link->y / CSFI) + Renderer::getInstance()->sprites.sprites[link->sprite].frame[link->frame].dir[dir].actionpoint.y) * CSFI;
    o->dir = dir;
  }
  else
  {
    o->Delete();
  }
}

void onspawn_snap_to_ground(Object *o)
{
  o->SnapToGround();
}

void onspawn_set_frame_from_id2(Object *o)
{
  o->frame = o->id2;
}
