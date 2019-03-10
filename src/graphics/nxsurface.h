
#ifndef _NXSURFACE_H
#define _NXSURFACE_H

#include "../common/basics.h"

#include <SDL.h>
#include <string>

// scaling factor for the graphics
extern int SCALE;

class NXSurface;
extern NXSurface *screen;

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

struct NXRect : public SDL_Rect
{
  inline NXRect() {}

  inline NXRect(int x, int y, int w, int h)
  {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }

  inline void Set(int x, int y, int w, int h)
  {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
};

typedef SDL_PixelFormat NXFormat;

class NXSurface
{
public:
  NXSurface();
  NXSurface(int wd, int ht, NXFormat *format = screen->Format());
  static NXSurface *createScreen(int wd, int ht, Uint32 pixel_format);
  // NXSurface(SDL_Surface *from_sfc, bool freesurface=true);
  // NXSurface(SDL_Renderer *renderer);
  ~NXSurface();

  bool AllocNew(int wd, int ht, NXFormat *format = screen->Format());
  bool LoadImage(const std::string &pbm_name, bool use_colorkey = false, int use_display_format = -1);
  static NXSurface *FromFile(const std::string &pbm_name, bool use_colorkey = false, int use_display_format = -1);

  // blitting
  void DrawSurface(NXSurface *src, int dstx, int dsty);
  void DrawSurface(NXSurface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);
  void BlitPatternAcross(NXSurface *src, int x_dst, int y_dst, int y_src, int height);

  void DrawBatchBegin(size_t max_count);
  void DrawBatchAdd(NXSurface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);
  void DrawBatchAdd(NXSurface *src, int dstx, int dsty);
  void DrawBatchAddPatternAcross(NXSurface *sfc, int x_dst, int y_dst, int y_src, int height);
  void DrawBatchEnd();

  // graphics primitives
  void DrawLine(int x1, int y1, int x2, int y2, NXColor color);

  void DrawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
  void DrawRect(int x1, int y1, int x2, int y2, NXColor color);
  void DrawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
  void DrawRect(NXRect *rect, NXColor color);

  void FillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
  void FillRect(int x1, int y1, int x2, int y2, NXColor color);
  void TintRect(int x1, int y1, int x2, int y2, NXColor color, uint16_t alpha);
  void FillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
  void FillRect(NXRect *rect, NXColor color);
  void ClearRect(int x1, int y1, int x2, int y2);
  void ClearRect(NXRect *rect);
  void Clear(uint8_t r, uint8_t g, uint8_t b);

  void DrawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
  void DrawPixel(int x, int y, NXColor color);

  // misc
  void set_clip_rect(int x, int y, int w, int h);
  void set_clip_rect(NXRect *rect);
  void clear_clip_rect();
  bool is_set_clip() const;
  void clip(SDL_Rect &srcrect,SDL_Rect &dstrect) const;
  void clip_scaled(SDL_Rect &srcrect,SDL_Rect &dstrect) const;

  int Width();
  int Height();
  NXFormat *Format();

  void Flip();

  static void SetScale(int factor);

  void SetAsTarget(bool enable);

  void setFormat(NXFormat const *format);
  void setPixelFormat(Uint32 format);

private:
  void Free();

  SDL_Texture *fTexture;
  int tex_w;
  int tex_h;
  NXFormat tex_format;

  bool need_clip;
  SDL_Rect clip_rect;

public:
  int alpha = 255;
};

void inline NXSurface::DrawRect(int x1, int y1, int x2, int y2, NXColor color)
{
  DrawRect(x1, y1, x2, y2, color.r, color.g, color.b);
}

void inline NXSurface::DrawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{
  DrawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b);
}

void inline NXSurface::DrawRect(NXRect *rect, NXColor color)
{
  DrawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b);
}

void inline NXSurface::FillRect(int x1, int y1, int x2, int y2, NXColor color)
{
  FillRect(x1, y1, x2, y2, color.r, color.g, color.b);
}

void inline NXSurface::FillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{
  FillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b);
}

void inline NXSurface::FillRect(NXRect *rect, NXColor color)
{
  FillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b);
}

void inline NXSurface::ClearRect(NXRect *rect)
{
  ClearRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1));
}

void inline NXSurface::DrawPixel(int x, int y, NXColor color)
{
  DrawPixel(x, y, color.r, color.g, color.b);
}

#endif
