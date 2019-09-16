#ifndef _G_TYPES_H
#define _G_TYPES_H

#include <cstdint>
#include <SDL.h>

struct NXColor
{
  uint8_t r, g, b;

  NXColor()
      : r(0)
      , g(0)
      , b(0)
  {
  }

  NXColor(uint8_t rr, uint8_t gg, uint8_t bb)
      : r(rr)
      , g(gg)
      , b(bb)
  {
  }

  NXColor(uint32_t hexcolor)
  {
    r = hexcolor >> 16;
    g = hexcolor >> 8;
    b = hexcolor;
  }
};

typedef SDL_Rect NXRect;

#endif
