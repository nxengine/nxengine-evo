#include "weapons.h"

#include "../../ObjManager.h"
#include "../../caret.h"
#include "../../common/stat.h"
#include "../../game.h"
#include "../../graphics/sprites.h"
#include "../../map.h"
#include "../../player.h"
#include "../../sound/SoundManager.h"
#include "../sym/smoke.h"

/*
void c------------------------------() {}
*/

// a convenience function which does some stuff common to a basic weapon--
// damage enemies, hit walls, and ttl dispersion.
// if it deletes the shot, returns nonzero.
uint8_t run_shot(Object *o, bool destroys_blocks)
{
  if (damage_enemies(o))
  {
    o->Delete();
    return RS_HIT_ENEMY;
  }

  if (IsBlockedInShotDir(o))
  {
    shot_spawn_effect(o, EFFECT_STARSOLID);

    if (destroys_blocks)
    {
      if (!shot_destroy_blocks(o))
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SHOT_HIT);
    }

    o->Delete();
    return RS_HIT_WALL;
  }

  if (--o->shot.ttl < 0)
  {
    shot_spawn_effect(o, EFFECT_STARPOOF);
    o->Delete();
    return RS_TTL_EXPIRED;
  }

  return 0;
}

/*
void c------------------------------() {}
*/

// checks if the shot passed in has struck an enemy. if so, returns the enemy.
// optional parameter flags_to_exclude lets you pass through enemies with
// certain flags set (such as if you don't want to try to hurt invulnerable enemies).
Object *check_hit_enemy(Object *shot, uint32_t flags_to_exclude)
{
  Object *enemy;
  FOREACH_OBJECT(enemy)
  {
    if (enemy->flags & (FLAG_SHOOTABLE | FLAG_INVULNERABLE))
    {
      if ((enemy->flags & flags_to_exclude) == 0)
      {
        if (hitdetect(enemy, shot))
        {
          // can't hit an enemy by shooting up when standing on it
          // (added for omega battle but good probably in other times too)
          if (player->riding != enemy || shot->yinertia >= 0)
          {
            return enemy;
          }
        }
      }
    }
  }

  return NULL;
}

// checks if the player shot "o" has hit an enemy and handles damaging the enemy if so.
// also, if an enemy was damaged, returns it's handle.
// multi is for weapons such as the spur and fireball that can "plow through" enemies
// until their damage is exhausted.
Object *damage_enemies(Object *o, uint32_t flags_to_exclude)
{
  Object *enemy;

  // first check if we hit an enemy
  if ((enemy = check_hit_enemy(o, flags_to_exclude)))
  {
    if (enemy->flags & FLAG_INVULNERABLE)
    {
      shot_spawn_effect(o, EFFECT_STARSOLID);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TINK);
    }
    else
    {
      enemy->DealDelayedDamage(o->shot.damage, o);
    }

    return enemy;
  }

  return NULL;
}

// used by AoE weapons, damages all enemies within the bounding box,
// not just the first one found. Returns the number of enemies hit.
int damage_all_enemies_in_bb(Object *o, uint32_t flags_to_exclude, int x, int y, int range)
{
  Object *enemy;
  int count = 0;

  FOREACH_OBJECT(enemy)
  {
    if (enemy->flags & (FLAG_SHOOTABLE | FLAG_INVULNERABLE))
    {
      if ((enemy->flags & flags_to_exclude) == 0)
      {
        if (hitdetect_area(enemy, x, y, range))
        {
          if (enemy->flags & FLAG_INVULNERABLE)
          {
            shot_spawn_effect(o, EFFECT_STARSOLID);
            NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_TINK);
          }
          else
          {
            enemy->DealDelayedDamage(o->shot.damage, o);
          }

          count++;
        }
      }
    }
  }

  return count;
}

/*
void c------------------------------() {}
*/

// spawn an effect at a shot's center point
void shot_spawn_effect(Object *o, int effectno)
{
  int x = 0;
  int y = 0;

  // Nemesis shots are very long so just centering the star doesn't look right.
  // I could have gone off of aspect ratio but wanted to keep missiles the same
  // and not risk breaking any older weapons.
  if ((o->type == OBJ_NEMESIS_SHOT && o->shot.level != 2) || (o->type == OBJ_MGUN_LEADER))
  {
    switch (o->shot.dir)
    {
      case LEFT:
        x = o->x;
        y = o->CenterY();
        break;

      case RIGHT:
        x = (o->x + o->Width());
        y = o->CenterY();
        break;

      case UP:
        x = o->CenterX();
        y = o->y;
        break;

      case DOWN:
        x = o->CenterX();
        y = (o->y + o->Height());
        break;
    }
  }
  else
  {
    x = o->CenterX();
    y = o->CenterY();
  }

  if (effectno == EFFECT_STARSOLID || effectno == EFFECT_SPUR_HIT)
  { // embed it in the wall, instead of the spot where we hit at
    switch (o->shot.dir)
    {
      case RIGHT:
        x += 0x400;
        break;
      case LEFT:
        x -= 0x400;
        break;
      case UP:
        y -= 0x400;
        break;
      case DOWN:
        y += 0x400;
        break;
    }
  }

  effect(x, y, effectno);
}

void shot_dissipate(Object *o, int effectno)
{
  shot_spawn_effect(o, effectno);
  o->Delete();
}

/*
void c------------------------------() {}
*/
// called when certain kinds of shots hit a wall to see if it was a
// destroyable brick that they hit.
// this destroys star-blocks which are touching the shot.
// returns nonzero if any were destroyed.
bool shot_destroy_blocks(Object *o)
{
  int x, y;
  SIFPointList *plist;

  // select which pointlist to check based on which direction shot is traveling
  switch (o->shot.dir)
  {
    case LEFT:
      plist = &sprites[o->sprite].block_l;
      break;
    case RIGHT:
      plist = &sprites[o->sprite].block_r;
      break;
    case UP:
      plist = &sprites[o->sprite].block_u;
      break;
    case DOWN:
      plist = &sprites[o->sprite].block_d;
      break;
    default:
      return 0;
  }

  // see if we've hit a destroyable block
  if (o->CheckAttribute(plist, TA_DESTROYABLE, &x, &y))
  {
    map.tiles[x][y]--;
    SmokeCloudsSlow(((x * TILE_W) + (TILE_W / 2)) * CSFI, ((y * TILE_H) + (TILE_H / 2)) * CSFI, 4);

    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BLOCK_DESTROY);
    shot_spawn_effect(o, EFFECT_FISHY);
    return 1;
  }

  return 0;
}

/*
void c------------------------------() {}
*/

// returns nonzero if a shot is blocked in the direction it's traveling
bool IsBlockedInShotDir(Object *o)
{
  switch (o->shot.dir)
  {
    case LEFT:
      return o->blockl;
    case RIGHT:
      return o->blockr;
    case UP:
      return o->blocku;
    case DOWN:
      return o->blockd;
  }

  staterr("IsBlockedInShotDir(%x): invalid direction %d", o, o->shot.dir);
  return 0;
}
