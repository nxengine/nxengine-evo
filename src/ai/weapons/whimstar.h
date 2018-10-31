
#ifndef _WHIMSTAR_H
#define _WHIMSTAR_H

#include "../../object.h"

#define MAX_WHIMSTARS 3

struct Whimstar
{
  int x, y;
  int xinertia, yinertia;
};

struct WhimsicalStar
{
  Whimstar stars[MAX_WHIMSTARS];
  int nstars;
  int stariter;
};

void init_whimstar(WhimsicalStar *wh);
void add_whimstar(WhimsicalStar *wh);
void remove_whimstar(WhimsicalStar *wh);
void run_whimstar(WhimsicalStar *wh);
void draw_whimstars(WhimsicalStar *wh);
void ai_whimsical_star(Object *o);

#endif
