
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "nxsurface.h"
// (unscaled) screen size/video mode
#define SCREEN_WIDTH		320
#define SCREEN_HEIGHT		240

extern NXSurface *screen;
extern const NXColor DK_BLUE;
extern const NXColor BLACK;
extern const NXColor CLEAR;
extern bool use_palette;

namespace Graphics
{
	bool init(int resolution);
	void close();
	
	bool InitVideo();
	void SetFullscreen(bool enable);
	bool SetResolution(int factor, bool restoreOnFailure=true);
	const char **GetResolutions();
	bool FlushAll();
	
	// ---------------------------------------
	
	void CopySpriteToTile(int spr, int tileno, int offset_x, int offset_y);
	void ShowLoadingScreen();
	
	void BlitSurface(NXSurface *src, NXRect *srcrect, NXSurface *dst, NXRect *dstrect);
	
	// these are all just convenience wrappers around the equivalent
	// NXSurface member functions, most of which are set to target the screen.
	void DrawSurface(NXSurface *src, int x, int y);
	void DrawSurface(NXSurface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);
	
	void BlitPatternAcross(NXSurface *sfc, int x_dst, int y_dst, int y_src, int height);
	
	
	void ClearScreen(NXColor color);
	void ClearScreen(uint8_t r, uint8_t g, uint8_t b);


	void DrawRect(int x1, int y1, int x2, int y2, NXColor color);
	void FillRect(int x1, int y1, int x2, int y2, NXColor color);
	void DrawPixel(int x, int y, NXColor color);
	
	void DrawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
	void FillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
	void DrawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
	
	void set_clip_rect(int x, int y, int w, int h);
	void set_clip_rect(NXRect *rect);
	void clear_clip_rect();
	
	void SetDrawTarget(NXSurface *surface);
};

#endif
