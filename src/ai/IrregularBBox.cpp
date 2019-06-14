
#include "IrregularBBox.h"

#include "../ObjManager.h"
#include "../autogen/sprites.h"
#include "../Utils/Logger.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../nx.h"
using namespace NXE::Graphics;

bool IrregularBBox::init(Object *associatedObject, int max_rectangles)
{
  memset(&bbox, 0, sizeof(bbox));
  this->num_bboxes   = max_rectangles;
  this->assoc_object = associatedObject;

  if (num_bboxes >= IB_MAX_BBOXES)
  {
    num_bboxes = IB_MAX_BBOXES;
    LOG_ERROR("IrregularBBox::Init(): too many rectangles ({} given): max is {}", max_rectangles, IB_MAX_BBOXES);
    return 1;
  }

  for (int i = 0; i < num_bboxes; i++)
  {
    bbox[i]            = CreateObject(0, 0, OBJ_BBOX_PUPPET);
    bbox[i]->sprite    = SPR_BBOX_PUPPET_1 + i;
    bbox[i]->hp        = 1000;
    bbox[i]->damage    = 0;
    bbox[i]->invisible = true;
  }

  return 0;
}

void IrregularBBox::destroy()
{
  for (int i = 0; i < num_bboxes; i++)
  {
    bbox[i]->Delete();
    bbox[i] = NULL;
  }

  num_bboxes   = 0;
  assoc_object = NULL;
}

/*
void c------------------------------() {}
*/

void IrregularBBox::set_damage(int dmg)
{
  for (int i = 0; i < num_bboxes; i++)
    bbox[i]->damage = dmg;
}

// link damage hits on the bbox to damage on it's associated object, o
void IrregularBBox::transmit_hits()
{
  if (!assoc_object)
    return;

  for (int i = 0; i < num_bboxes; i++)
  {
    if (bbox[i]->hp < 1000)
    {
      assoc_object->DealDamage(1000 - bbox[i]->hp);
      bbox[i]->hp = 1000;
    }
  }
}

/*
void c------------------------------() {}
*/

void IrregularBBox::place(void (*placefunc)(void *userparm), void *userparm)
{
  // first assume all disabled
  for (int i = 0; i < num_bboxes; i++)
  {
    bbox[i]->y = -1024; // effectively disables it
  }

  // ask object to place it's bboxes as it wishes for this frame
  // by using set_bbox().
  (*placefunc)(userparm);
}

void IrregularBBox::set_bbox(int index, int x, int y, int w, int h, uint32_t flags)
{
  if (index < 0 || index >= num_bboxes)
  {
    LOG_ERROR("IrregularBBox::set_bbox: index out of range: {}", index);
    return;
  }

  Object *box = bbox[index];

  // coordinates passed in here are for the right-facing frame,
  // if we are currently left-facing then flip them.
  if (assoc_object->dir == LEFT)
    x = Renderer::getInstance()->sprites.sprites[assoc_object->sprite].w - x - w;

  box->x = assoc_object->x + (x * CSFI);
  box->y = assoc_object->y + (y * CSFI);

  Renderer::getInstance()->sprites.sprites[box->sprite].bbox[0].x1 = 0;
  Renderer::getInstance()->sprites.sprites[box->sprite].bbox[0].y1 = 0;
  Renderer::getInstance()->sprites.sprites[box->sprite].bbox[0].x2 = (w - 1);
  Renderer::getInstance()->sprites.sprites[box->sprite].bbox[0].y2 = (h - 1);

  box->flags &= ~(FLAG_SHOOTABLE | FLAG_INVULNERABLE);
  box->flags |= flags;

  // nitpicking perfect sync for this frame since we are not aftermove
  box->x += assoc_object->xinertia;
  box->y += assoc_object->yinertia;
}
