
#ifndef _IRREGULARBBOX_H
#define _IRREGULARBBOX_H
#include "../object.h"

#define IB_MAX_BBOXES 4

class IrregularBBox
{
public:
  bool init(Object *associatedObject, int max_rectangles);
  void destroy();

  void set_damage(int dmg);
  void transmit_hits();

  void place(void (*placefunc)(void *userparm), void *userparm);
  void set_bbox(int index, int x, int y, int w, int h, uint32_t flags);

private:
  Object *bbox[IB_MAX_BBOXES];
  int num_bboxes;
  Object *assoc_object;
};

#endif
