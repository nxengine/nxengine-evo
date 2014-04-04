
#ifndef _NXSURFACE_H
#define _NXSURFACE_H

#include <SDL/SDL.h>
#include "../common/basics.h"

// scaling factor for the graphics
#ifdef CONFIG_MUTABLE_SCALE
	extern int SCALE;
#else
	#define SCALE		1
#endif

SDL_Surface *palette_add(SDL_Surface *sfc);

struct NXSurface;
extern NXSurface *screen;

struct NXColor
{
	uint8_t r, g, b;
	
	NXColor(uint8_t rr, uint8_t gg, uint8_t bb)
		: r(rr), g(gg), b(bb)
	{ }
	
	NXColor(uint32_t hexcolor)
	{
		r = hexcolor >> 16;
		g = hexcolor >> 8;
		b = hexcolor;
	}
};

struct NXRect : public SDL_Rect
{
	inline NXRect() { }
	
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

typedef SDL_PixelFormat	NXFormat;


class NXSurface
{
public:
	NXSurface();
	NXSurface(int wd, int ht, NXFormat *format = screen->Format());
	NXSurface(SDL_Surface *from_sfc, bool freesurface=true);
	~NXSurface();
	
	bool AllocNew(int wd, int ht, NXFormat *format = screen->Format());
	bool LoadImage(const char *pbm_name, bool use_colorkey=false, int use_display_format=-1);
	static NXSurface *FromFile(const char *pbm_name, bool use_colorkey=false, int use_display_format=-1);
	
	// blitting
	void DrawSurface(NXSurface *src, int dstx, int dsty);
	void DrawSurface(NXSurface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);
	void BlitPatternAcross(NXSurface *src, int x_dst, int y_dst, int y_src, int height);
	
	// graphics primitives
	void DrawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
	void DrawRect(int x1, int y1, int x2, int y2, NXColor color);
	void DrawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
	void DrawRect(NXRect *rect, NXColor color);
	
	void FillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
	void FillRect(int x1, int y1, int x2, int y2, NXColor color);
	void FillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
	void FillRect(NXRect *rect, NXColor color);
	void Clear(uint8_t r, uint8_t g, uint8_t b);
	
	void DrawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
	void DrawPixel(int x, int y, NXColor color);
	
	// misc
	void set_clip_rect(int x, int y, int w, int h);
	void set_clip_rect(NXRect *rect);
	void clear_clip_rect();
	
	int Width();
	int Height();
	void EnableColorKey();
	NXFormat *Format();
	
	void Flip();
	SDL_Surface *GetSDLSurface() { return fSurface; }
	
	static void SetScale(int factor);
	
private:
	static SDL_Surface *Scale(SDL_Surface *original, int factor, bool use_colorkey, bool free_original, bool use_display_format);
	static void Scale8(SDL_Surface *src, SDL_Surface *dst, int factor);
	
	inline uint32_t MapColor(uint8_t r, uint8_t g, uint8_t b);
	void Free();
	
	SDL_Surface *fSurface;
	bool fFreeSurface;
};

void inline
NXSurface::DrawRect(int x1, int y1, int x2, int y2, NXColor color)
{ DrawRect(x1, y1, x2, y2, color.r, color.g, color.b); }

void inline
NXSurface::DrawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{ DrawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b); }

void inline
NXSurface::DrawRect(NXRect *rect, NXColor color)
{ DrawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b); }


void inline
NXSurface::FillRect(int x1, int y1, int x2, int y2, NXColor color)
{ FillRect(x1, y1, x2, y2, color.r, color.g, color.b); }

void inline
NXSurface::FillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{ FillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b); }

void inline
NXSurface::FillRect(NXRect *rect, NXColor color)
{ FillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b); }


void inline
NXSurface::DrawPixel(int x, int y, NXColor color)
{ DrawPixel(x, y, color.r, color.g, color.b); }


#endif

