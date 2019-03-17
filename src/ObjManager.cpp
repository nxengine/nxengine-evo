
#include "ObjManager.h"

#include "autogen/sprites.h"
#include "common/llist.h"
#include "game.h"
#include "graphics/Renderer.h"
#include "map.h"
#include "nx.h"
#include "object.h"
#include "player.h"

using namespace NXE::Graphics;

static Object ZERO_OBJECT;
static Player ZERO_PLAYER;

Object *firstobject = NULL, *lastobject = NULL;
Object *lowestobject = NULL, *highestobject = NULL;

Object *bullets[64];

/*
void c------------------------------() {}
*/

Object *CreateObject(int x, int y, int type, int xinertia, int yinertia, int dir, Object *linkedobject,
                     uint32_t createflags)
{
  Object *o;

  // create the structure
  if (type != OBJ_PLAYER)
  {
    o  = new Object;
    *o = ZERO_OBJECT; // safely clears all members
  }
  else
  {
    Player *p = new Player;
    *p        = ZERO_PLAYER;
    o         = (Object *)p;
  }

  // initialize
  o->SetType(type);
  o->flags      = objprop[type].defaultflags;
  o->DamageText = new FloatText(SPR_REDNUMBERS);

  o->x            = x - (Renderer::getInstance()->sprites.sprites[o->sprite].spawn_point.x * CSFI);
  o->y            = y - (Renderer::getInstance()->sprites.sprites[o->sprite].spawn_point.y * CSFI);
  o->dir          = dir;
  o->xinertia     = xinertia;
  o->yinertia     = yinertia;
  o->linkedobject = linkedobject;

  // add into list
  LL_ADD_END(o, prev, next, firstobject, lastobject);
  LL_ADD_END(o, lower, higher, lowestobject, highestobject);

  // set it's initial blocked states, but do not update blockedstates on objects starting
  // with nullsprite-- the reason is for objects whose sprite is set after being spawned
  if (o->sprite != SPR_NULL)
    o->UpdateBlockStates(ALLDIRMASK);

  if (!(createflags & CF_NO_SPAWN_EVENT))
    o->OnSpawn();

  return o;
}

// bullets are stored in separate array
Object *CreateBullet(int x, int y, int type)
{
  Object *o;

  o  = new Object;
  *o = ZERO_OBJECT; // safely clears all members

  // initialize
  o->SetType(type);
  o->flags      = objprop[type].defaultflags;
  o->DamageText = new FloatText(SPR_REDNUMBERS);

  o->x            = x;
  o->y            = y;
  o->dir          = RIGHT;
  o->xinertia     = 0;
  o->yinertia     = 0;
  o->linkedobject = NULL;

  // add into list
  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] == NULL)
    {
      bullets[i] = o;
      break;
    }
  }
  LL_ADD_END(o, lower, higher, lowestobject, highestobject);

  return o;
}

Object *CreateObject(int x, int y, int type)
{
  return CreateObject(x, y, type, 0, 0, RIGHT, NULL, CF_DEFAULT);
}

/*
void c------------------------------() {}
*/

// update the blocked states of all objects
void Objects::UpdateBlockStates(void)
{
  Object *o = firstobject;
  while (o)
  {
    o->lastblockl = o->blockl;
    o->lastblockr = o->blockr;
    o->lastblocku = o->blocku;
    o->lastblockd = o->blockd;

    o->UpdateBlockStates(ALLDIRMASK);
    o = o->next;
  }

  for (int i = 0; i < 64; i++)
  {
    o = bullets[i];
    if (o != NULL)
    {
      o->lastblockl = o->blockl;
      o->lastblockr = o->blockr;
      o->lastblocku = o->blocku;
      o->lastblockd = o->blockd;

      o->UpdateBlockStates(ALLDIRMASK);
    }
  }
}

// returns true if the bounding boxes of the two given objects are touching
bool hitdetect(Object *o1, Object *o2)
{
  SIFSprite *s1, *s2;
  int32_t rect1x1, rect1y1, rect1x2, rect1y2;
  int32_t rect2x1, rect2y1, rect2x2, rect2y2;

  // get the sprites used by the two objects
  s1 = o1->Sprite();
  s2 = o2->Sprite();

  // get the bounding rectangle of the first object
  rect1x1 = o1->x + (s1->bbox[o1->dir].x1 * CSFI);
  rect1x2 = o1->x + (s1->bbox[o1->dir].x2 * CSFI);
  rect1y1 = o1->y + (s1->bbox[o1->dir].y1 * CSFI);
  rect1y2 = o1->y + (s1->bbox[o1->dir].y2 * CSFI);

  // get the bounding rectangle of the second object
  rect2x1 = o2->x + (s2->bbox[o2->dir].x1 * CSFI);
  rect2x2 = o2->x + (s2->bbox[o2->dir].x2 * CSFI);
  rect2y1 = o2->y + (s2->bbox[o2->dir].y1 * CSFI);
  rect2y2 = o2->y + (s2->bbox[o2->dir].y2 * CSFI);

  // find out if the rectangles overlap
  if ((rect1x1 < rect2x1) && (rect1x2 < rect2x1))
    return false;
  if ((rect1x1 > rect2x2) && (rect1x2 > rect2x2))
    return false;
  if ((rect1y1 < rect2y1) && (rect1y2 < rect2y1))
    return false;
  if ((rect1y1 > rect2y2) && (rect1y2 > rect2y2))
    return false;

  return true;
}

// returns true if the bounding boxes of the two given objects are touching
bool hitdetect_shot(Object *o1, Object *o2)
{
  SIFSprite *s1, *s2;
  int32_t rect1x1, rect1y1, rect1x2, rect1y2;
  int32_t rect2x1, rect2y1, rect2x2, rect2y2;

  // get the sprites used by the two objects
  s1 = o1->Sprite();
  s2 = o2->Sprite();

  // get the bounding rectangle of the first object
  rect1x1 = o1->x + (s1->bbox[o1->dir].x1 * CSFI);
  rect1x2 = o1->x + (s1->bbox[o1->dir].x2 * CSFI);
  rect1y1 = o1->y + (s1->bbox[o1->dir].y1 * CSFI);
  rect1y2 = o1->y + (s1->bbox[o1->dir].y2 * CSFI);

  // get the bounding rectangle of the second object
  rect2x1 = o2->x + (s2->bbox[o2->shot.dir].x1 * CSFI);
  rect2x2 = o2->x + (s2->bbox[o2->shot.dir].x2 * CSFI);
  rect2y1 = o2->y + (s2->bbox[o2->shot.dir].y1 * CSFI);
  rect2y2 = o2->y + (s2->bbox[o2->shot.dir].y2 * CSFI);

  // find out if the rectangles overlap
  if ((rect1x1 < rect2x1) && (rect1x2 < rect2x1))
    return false;
  if ((rect1x1 > rect2x2) && (rect1x2 > rect2x2))
    return false;
  if ((rect1y1 < rect2y1) && (rect1y2 < rect2y1))
    return false;
  if ((rect1y1 > rect2y2) && (rect1y2 > rect2y2))
    return false;

  return true;
}

bool hitdetect_area(Object *o1, int x, int y, int range)
{
  SIFSprite *s1;
  int32_t rect1x1, rect1y1, rect1x2, rect1y2;
  int32_t rect2x1, rect2y1, rect2x2, rect2y2;

  // get the sprites used by the two objects
  s1 = o1->Sprite();

  // get the bounding rectangle of the first object
  rect1x1 = o1->x + (s1->bbox[o1->dir].x1 * CSFI);
  rect1y1 = o1->y + (s1->bbox[o1->dir].y1 * CSFI);

  rect1x2 = o1->x + (s1->bbox[o1->dir].x2 * CSFI);
  rect1y2 = o1->y + (s1->bbox[o1->dir].y2 * CSFI);

  // get the bounding rectangle of the second object
  rect2x1 = x - (range * CSFI);
  rect2x2 = x + (range * CSFI);
  rect2y1 = y - (range * CSFI);
  rect2y2 = y + (range * CSFI);

  // find out if the rectangles overlap
  if ((rect1x1 < rect2x1) && (rect1x2 < rect2x1))
    return false;
  if ((rect1x1 > rect2x2) && (rect1x2 > rect2x2))
    return false;
  if ((rect1y1 < rect2y1) && (rect1y2 < rect2y1))
    return false;
  if ((rect1y1 > rect2y2) && (rect1y2 > rect2y2))
    return false;

  return true;
}

bool hitdetect_damage_player(Object *o1)
{
  SIFSprite *s1;
  int32_t rect1x1, rect1y1, rect1x2, rect1y2;
  int32_t rect2x1, rect2y1, rect2x2, rect2y2;

  // get the sprites used by the two objects
  s1 = o1->Sprite();

  // get the bounding rectangle of the first object
  rect1x1 = o1->x + (s1->bbox[o1->dir].x1 * CSFI);
  rect1x2 = o1->x + (s1->bbox[o1->dir].x2 * CSFI);
  rect1y1 = o1->y + (s1->bbox[o1->dir].y1 * CSFI);
  rect1y2 = o1->y + (s1->bbox[o1->dir].y2 * CSFI);

  // get the bounding rectangle of the second object
  rect2x1 = player->x + (4 * CSFI);
  rect2x2 = player->x + (4 * CSFI);
  rect2y1 = player->y + (4 * CSFI);
  rect2y2 = player->y + (4 * CSFI);

  // find out if the rectangles overlap
  if ((rect1x1 < rect2x1) && (rect1x2 < rect2x1))
    return false;
  if ((rect1x1 > rect2x2) && (rect1x2 > rect2x2))
    return false;
  if ((rect1y1 < rect2y1) && (rect1y2 < rect2y1))
    return false;
  if ((rect1y1 > rect2y2) && (rect1y2 > rect2y2))
    return false;

  return true;
}

// returns true if the solidity boxes of the two given objects are touching
bool solidhitdetect(Object *o1, Object *o2)
{
  SIFSprite *s1, *s2;
  int32_t rect1x1, rect1y1, rect1x2, rect1y2;
  int32_t rect2x1, rect2y1, rect2x2, rect2y2;

  // get the sprites used by the two objects
  s1 = o1->Sprite();
  s2 = o2->Sprite();

  // get the bounding rectangle of the first object
  rect1x1 = o1->x + (s1->solidbox.x1 * CSFI);
  rect1x2 = o1->x + (s1->solidbox.x2 * CSFI);
  rect1y1 = o1->y + (s1->solidbox.y1 * CSFI);
  rect1y2 = o1->y + (s1->solidbox.y2 * CSFI);

  // get the bounding rectangle of the second object
  rect2x1 = o2->x + (s2->solidbox.x1 * CSFI);
  rect2x2 = o2->x + (s2->solidbox.x2 * CSFI);
  rect2y1 = o2->y + (s2->solidbox.y1 * CSFI);
  rect2y2 = o2->y + (s2->solidbox.y2 * CSFI);

  // find out if the rectangles overlap
  if ((rect1x1 < rect2x1) && (rect1x2 < rect2x1))
    return false;
  if ((rect1x1 > rect2x2) && (rect1x2 > rect2x2))
    return false;
  if ((rect1y1 < rect2y1) && (rect1y2 < rect2y1))
    return false;
  if ((rect1y1 > rect2y2) && (rect1y2 > rect2y2))
    return false;

  return true;
}

/*
void c------------------------------() {}
*/

// runs all entity AI routines
void Objects::RunAI(void)
{
  Object *o;

  // because we handle objects in order of their creation and have a separate list
  // for display order, we can't ever run AI twice in a frame because of z-order
  // rearrangement, and 2) objects created by other objects are added to the end of
  // the list and given a chance to run their AI routine before being displayed.

  // run bullet ai first
  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL)
    {
      bullets[i]->RunAI();
    }
  }

  FOREACH_OBJECT(o)
  {
    if (!o->deleted)
    {
      o->RunAI();
    }
  }
}

void Objects::RunAfterMove(void)
{
  Object *o;

  FOREACH_OBJECT(o)
  {
    if (!o->deleted)
    {
      o->OnAftermove();
    }
  }

  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL)
    {
      bullets[i]->OnAftermove();
    }
  }

  // deal summary bullet damage at the end of the frame
  FOREACH_OBJECT(o)
  {
    if (!o->deleted)
    {
      if (o->damaged)
      {
        o->DealDamage(o->damaged, o->whohit);
        o->damaged = 0;
      }
    }
  }
}

// the most important thing it does is apply x/y inertia to the objects.
void Objects::PhysicsSim(void)
{
  Object *o;
  int xinertia, yinertia;

  FOREACH_OBJECT(o)
  {
    if (o != player && !o->deleted) // player is moved in PDoPhysics
    {
      if (!(o->flags & FLAG_IGNORE_SOLID) && !(o->nxflags & NXFLAG_NO_RESET_YINERTIA))
      {
        if (o->blockd && o->yinertia > 0)
          o->yinertia = 0;
        if (o->blocku && o->yinertia < 0)
          o->yinertia = 0;
      }

      // apply inertia to X,Y position
      xinertia = o->xinertia;
      yinertia = o->yinertia;
      if (o->shaketime)
      {
        if (o->nxflags & NXFLAG_SLOW_X_WHEN_HURT)
          xinertia >>= 1;
        if (o->nxflags & NXFLAG_SLOW_Y_WHEN_HURT)
          yinertia >>= 1;
      }

      o->apply_xinertia(xinertia);
      o->apply_yinertia(yinertia);

      // flag_solid_brick objects push player as they move
      if (o->flags & FLAG_SOLID_BRICK)
      {
        o->PushPlayerOutOfWay(xinertia, yinertia);
      }
      else if (o->damage > 0)
      {
        // have enemies hurt you when you touch them
        // (solid-brick objects do this in PHandleSolidBrickObjects)
        // SOLID_MUSHY objects doen't let the player sink enough for a smaller hitbox to collide
        // so run detection with a bigger one.
        // this can be "easily" tested on balcony: you should be able to stand on mimiga, but not on Igor.
        if (o->flags & FLAG_SOLID_MUSHY)
        {
          if (hitdetect(o, player))
            o->DealContactDamage();
        }
        else
        {
          if (hitdetect_damage_player(o))
            o->DealContactDamage();
        }
      }
    }
  }

  for (int i = 0; i < 64; i++)
  {
    o = bullets[i];
    if (o != NULL && o != player && !o->deleted) // player is moved in PDoPhysics
    {
      if (!(o->flags & FLAG_IGNORE_SOLID) && !(o->nxflags & NXFLAG_NO_RESET_YINERTIA))
      {
        if (o->blockd && o->yinertia > 0)
          o->yinertia = 0;
        if (o->blocku && o->yinertia < 0)
          o->yinertia = 0;
      }

      // apply inertia to X,Y position
      xinertia = o->xinertia;
      yinertia = o->yinertia;
      if (o->shaketime)
      {
        if (o->nxflags & NXFLAG_SLOW_X_WHEN_HURT)
          xinertia >>= 1;
        if (o->nxflags & NXFLAG_SLOW_Y_WHEN_HURT)
          yinertia >>= 1;
      }

      o->apply_xinertia(xinertia);
      o->apply_yinertia(yinertia);

      // flag_solid_brick objects push player as they move
      if (o->flags & FLAG_SOLID_BRICK)
      {
        o->PushPlayerOutOfWay(xinertia, yinertia);
      }
      else if (o->damage > 0)
      {
        // have enemies hurt you when you touch them
        // (solid-brick objects do this in PHandleSolidBrickObjects)
        // SOLID_MUSHY objects doen't let the player sink enough for a smaller hitbox to collide
        // so run detection with a bigger one.
        // this can be "easily" tested on balcony: you should be able to stand on mimiga, but not on Igor.
        if (o->flags & FLAG_SOLID_MUSHY)
        {
          if (hitdetect(o, player))
            o->DealContactDamage();
        }
        else
        {
          if (hitdetect_damage_player(o))
            o->DealContactDamage();
        }
      }
    }
  }
}

/*
void c------------------------------() {}
*/

// returns how many objects exist of the given type
int Objects::CountType(int objtype)
{
  int count = 0;
  Object *o;

  FOREACH_OBJECT(o)
  {
    if (o->type == objtype)
      count++;
  }

  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL && bullets[i]->type == objtype)
      count++;
  }

  return count;
}

// returns the first object of type objtype or NULL
Object *Objects::FindByType(int objtype)
{
  Object *o;
  FOREACH_OBJECT(o)
  {
    if (o->type == objtype)
      return o;
  }

  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL && bullets[i]->type == objtype)
      return bullets[i];
  }

  return NULL;
}

/*
void c------------------------------() {}
*/

// free objects deleted earlier via ObjDel
void Objects::CullDeleted(void)
{
  Object *o, *next;

  o = firstobject;
  while (o)
  {
    next = o->next;

    if (o->deleted)
    {
      o->Destroy();
    }

    o = next;
  }
  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL && bullets[i]->deleted)
    {
      bullets[i]->Destroy();
      bullets[i] = NULL;
    }
  }
}

// deletes all objects. if delete_player is true, also deletes the player.
// used by load_pxe to reset the game in preperation for loading a new maplayer->
void Objects::DestroyAll(bool delete_player)
{
  Object *o, *next;

  o = firstobject;
  while (o)
  {
    next = o->next;

    if (o != player)
    {
      o->Destroy();
    }

    o = next;
  }

  for (int i = 0; i < 64; i++)
  {
    if (bullets[i] != NULL)
    {
      bullets[i]->Destroy();
      bullets[i] = NULL;
    }
  }

  // must do this last to avoid crashes as player ptr gets invalidated
  if (delete_player)
  {
    player->Destroy();
  }

  memset(ID2Lookup, 0, sizeof(ID2Lookup));
}
