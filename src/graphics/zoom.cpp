/*
 * Based on SDL2_gfx
 * http://www.ferzkopp.net/Software/SDL2_gfx/Docs/html/index.html
 */

#ifdef WIN32
#include <windows.h>
#endif

#include "zoom.h"

#include <stdlib.h>
#include <string.h>

typedef struct tColorRGBA
{
  Uint8 r;
  Uint8 g;
  Uint8 b;
  Uint8 a;
} tColorRGBA;

typedef struct tColorY
{
  Uint8 y;
} tColorY;

#define GUARD_ROWS (2)

#define VALUE_LIMIT 0.001

Uint32 _colorkey(SDL_Surface *src)
{
  Uint32 key = 0;
  SDL_GetColorKey(src, &key);
  return key;
}

int _zoomSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst)
{
  int x, y, sx, sy, ssx, ssy, *sax, *say, *csax, *csay, *salast, csx, csy, sstep;
  tColorRGBA *sp, *csp, *dp;
  int spixelgap, dgap;

  /*
   * Allocate memory for row/column increments
   */
  if ((sax = (int *)malloc((dst->w + 1) * sizeof(Uint32))) == NULL)
  {
    return (-1);
  }
  if ((say = (int *)malloc((dst->h + 1) * sizeof(Uint32))) == NULL)
  {
    free(sax);
    return (-1);
  }

  /*
   * Precalculate row increments
   */
  sx = (int)(65536.0 * (float)(src->w) / (float)(dst->w));
  sy = (int)(65536.0 * (float)(src->h) / (float)(dst->h));

  /* Maximum scaled source size */
  ssx = (src->w << 16) - 1;
  ssy = (src->h << 16) - 1;

  /* Precalculate horizontal row increments */
  csx  = 0;
  csax = sax;
  for (x = 0; x <= dst->w; x++)
  {
    *csax = csx;
    csax++;
    csx += sx;

    /* Guard from overflows */
    if (csx > ssx)
    {
      csx = ssx;
    }
  }

  /* Precalculate vertical row increments */
  csy  = 0;
  csay = say;
  for (y = 0; y <= dst->h; y++)
  {
    *csay = csy;
    csay++;
    csy += sy;

    /* Guard from overflows */
    if (csy > ssy)
    {
      csy = ssy;
    }
  }

  sp        = (tColorRGBA *)src->pixels;
  dp        = (tColorRGBA *)dst->pixels;
  dgap      = dst->pitch - dst->w * 4;
  spixelgap = src->pitch / 4;

  /*
   * Non-Interpolating Zoom
   */
  csay = say;
  for (y = 0; y < dst->h; y++)
  {
    csp  = sp;
    csax = sax;
    for (x = 0; x < dst->w; x++)
    {
      /*
       * Draw
       */
      *dp = *sp;
      /*
       * Advance source pointer x
       */
      salast = csax;
      csax++;
      sstep = (*csax >> 16) - (*salast >> 16);
      sp += sstep;
      /*
       * Advance destination pointer x
       */
      dp++;
    }
    /*
     * Advance source pointer y
     */
    salast = csay;
    csay++;
    sstep = (*csay >> 16) - (*salast >> 16);
    sstep *= spixelgap;
    sp = csp + sstep;
    /*
     * Advance destination pointer y
     */
    dp = (tColorRGBA *)((Uint8 *)dp + dgap);
  }

  /*
   * Remove temp arrays
   */
  free(sax);
  free(say);

  return (0);
}

int _zoomSurfaceY(SDL_Surface *src, SDL_Surface *dst)
{
  int x, y;
  Uint32 *sax, *say, *csax, *csay;
  int csx, csy;
  Uint8 *sp, *dp, *csp;
  int dgap;

  /*
   * Allocate memory for row increments
   */
  if ((sax = (Uint32 *)malloc((dst->w + 1) * sizeof(Uint32))) == NULL)
  {
    return (-1);
  }
  if ((say = (Uint32 *)malloc((dst->h + 1) * sizeof(Uint32))) == NULL)
  {
    free(sax);
    return (-1);
  }

  /*
   * Pointer setup
   */
  sp = csp = (Uint8 *)src->pixels;
  dp       = (Uint8 *)dst->pixels;
  dgap     = dst->pitch - dst->w;

  /*
   * Precalculate row increments
   */
  csx  = 0;
  csax = sax;
  for (x = 0; x < dst->w; x++)
  {
    csx += src->w;
    *csax = 0;
    while (csx >= dst->w)
    {
      csx -= dst->w;
      (*csax)++;
    }
    csax++;
  }
  csy  = 0;
  csay = say;
  for (y = 0; y < dst->h; y++)
  {
    csy += src->h;
    *csay = 0;
    while (csy >= dst->h)
    {
      csy -= dst->h;
      (*csay)++;
    }
    csay++;
  }

  /*
   * Draw
   */
  csay = say;
  for (y = 0; y < dst->h; y++)
  {
    csax = sax;
    sp   = csp;
    for (x = 0; x < dst->w; x++)
    {
      /*
       * Draw
       */
      *dp = *sp;
      /*
       * Advance source pointers
       */
      sp += (*csax);
      csax++;
      /*
       * Advance destination pointer
       */
      dp++;
    }
    /*
     * Advance source pointer (for row)
     */
    csp += ((*csay) * src->pitch);
    csay++;

    /*
     * Advance destination pointers
     */
    dp += dgap;
  }

  /*
   * Remove temp arrays
   */
  free(sax);
  free(say);

  return (0);
}

void _zoomSurfaceSize(int width, int height, double zoom, int *dstwidth, int *dstheight)
{
  if (zoom < VALUE_LIMIT)
  {
    zoom = VALUE_LIMIT;
  }

  *dstwidth  = (int)floor(((double)width * zoom) + 0.5);
  *dstheight = (int)floor(((double)height * zoom) + 0.5);

  if (*dstwidth < 1)
  {
    *dstwidth = 1;
  }
  if (*dstheight < 1)
  {
    *dstheight = 1;
  }
}

SDL_Surface *SDL_ZoomSurface(SDL_Surface *src, double zoom)
{
  SDL_Surface *rz_src;
  SDL_Surface *rz_dst;
  int dstwidth, dstheight;
  int is32bit;
  int i, src_converted;

  if (src == NULL)
    return (NULL);

  is32bit = (src->format->BitsPerPixel == 32);
  if ((is32bit) || (src->format->BitsPerPixel == 8))
  {
    rz_src        = src;
    src_converted = 0;
  }
  else
  {
    rz_src = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                  0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
                                  0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
    );

    if (rz_src == NULL)
    {
      return NULL;
    }

    SDL_BlitSurface(src, NULL, rz_src, NULL);
    src_converted = 1;
    is32bit       = 1;
  }

  // Get size of target
  _zoomSurfaceSize(rz_src->w, rz_src->h, zoom, &dstwidth, &dstheight);

  rz_dst = NULL;
  if (is32bit)
  {
    rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 32, rz_src->format->Rmask,
                                  rz_src->format->Gmask, rz_src->format->Bmask, rz_src->format->Amask);
  }
  else
  {
    rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 8, 0, 0, 0, 0);
  }

  if (rz_dst == NULL)
  {
    if (src_converted)
    {
      SDL_FreeSurface(rz_src);
    }
    return NULL;
  }

  // Adjust for guard rows
  rz_dst->h = dstheight;

  if (SDL_MUSTLOCK(rz_src))
  {
    SDL_LockSurface(rz_src);
  }

  if (is32bit)
  {
    _zoomSurfaceRGBA(rz_src, rz_dst);
  }
  else
  {
    // Copy palette and colorkey info
    for (i = 0; i < rz_src->format->palette->ncolors; i++)
    {
      rz_dst->format->palette->colors[i] = rz_src->format->palette->colors[i];
    }
    rz_dst->format->palette->ncolors = rz_src->format->palette->ncolors;

    _zoomSurfaceY(rz_src, rz_dst);
  }

  if (SDL_MUSTLOCK(rz_src))
  {
    SDL_UnlockSurface(rz_src);
  }

  if (src_converted)
  {
    SDL_FreeSurface(rz_src);
  }

  return (rz_dst);
}
