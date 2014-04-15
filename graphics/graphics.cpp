#include <cassert>
#include <algorithm>
// graphics routines
#include <SDL.h>

#include <stdlib.h>
#include "../config.h"
#include "graphics.h"
#include "tileset.h"
#include "sprites.h"
#include "font.h"
#include "palette.h"
#include "../dirnames.h"
#include "../map.h"

SDL_Window * window = NULL;
SDL_Renderer * renderer = NULL;


// (unscaled) screen size/video mode
int Graphics::SCREEN_WIDTH = 320;
int Graphics::SCREEN_HEIGHT = 240;


NXSurface *screen = NULL;				// created from SDL's screen
static NXSurface *drawtarget = NULL;	// target of DrawRect etc; almost always screen
bool use_palette = false;				// true if we are in an indexed-color video mode
int screen_bpp;

const NXColor DK_BLUE(0, 0, 0x21);		// the popular dk blue backdrop color
const NXColor BLACK(0, 0, 0);			// pure black, only works if no colorkey
//const NXColor CLEAR(0, 0, 0);			// the transparent/colorkey color

static bool is_fullscreen = false;
static int current_res = -1;

static NXSurface const* current_batch_drawtarget = NULL;

bool Graphics::init(int resolution)
{
	if (use_palette)
	{
		screen_bpp = 8;
	}
	else
	{
		screen_bpp = 16;	// the default
		
		// #ifndef __SDLSHIM__
		// const SDL_VideoInfo *info;
		
		// // it's faster if we create the SDL screen at the bpp of the real screen.
		// // max fps went from 120 to 160 on my X11 system this way.
		// if ((info = SDL_GetVideoInfo()))
		// {
		// 	stat("videoinfo: desktop bpp %d", info->vfmt->BitsPerPixel);
		// 	if (info->vfmt->BitsPerPixel > 8)
		// 		screen_bpp = info->vfmt->BitsPerPixel;
		// }
		// #endif
	}
	
	palette_reset();
	
	if (SetResolution(resolution, false))
		return 1;
	
	if (Tileset::Init())
		return 1;
	
	if (Sprites::Init())
		return 1;
	
	return 0;
}

void Graphics::close()
{
	stat("Graphics::Close()");
	SDL_ShowCursor(true);
	SDL_DestroyWindow(window); window = NULL;
}

bool Graphics::WindowVisible()
{
	Uint32 flags = SDL_GetWindowFlags(window);

	return (flags & SDL_WINDOW_SHOWN) && !(flags & SDL_WINDOW_MINIMIZED) // SDL_APPACTIVE
		&& (flags & SDL_WINDOW_INPUT_FOCUS);                              // SDL_APPINPUTFOCUS 
}

/*
void c------------------------------() {}
*/

bool Graphics::SelectResolution()
{
	SDL_DisplayMode curr, close;

	curr.w = 320;
	curr.h = 240;
	curr.driverdata = NULL;
	curr.refresh_rate = 0;
	curr.format = 0;
	
	// Getting biggest display mode
	// On iPhone 4S (retina device) there is two set of resolutions.
	// On iPad 2 - only one set.
	
	int displayModes = SDL_GetNumDisplayModes(0);
	
	if (displayModes <= 0)
	{
		staterr("SDL_GetNumDisplayModes modes count = %d, %s", displayModes, SDL_GetError());
		return true;
	}
	
	int maxw = 0;
	int maxi = 0;
	for (int i = 0; i < displayModes; ++i)
	{
		if (SDL_GetDisplayMode(0, i, &close))
		{
			staterr("SDL_GetDisplayMode %s", SDL_GetError());
			return true;
		}
		
		int w = close.w > close.h ? close.w : close.h;
		if (w > maxw)
		{
			maxw = w;
			maxi = i;
		}
	}
	
	if (SDL_GetDisplayMode(0, maxi, &close))
	{
		staterr("SDL_GetDisplayMode %s", SDL_GetError());
		return true;
	}
	
	if (close.w < close.h)
		std::swap(close.w, close.h);

	stat("closest w = %d, h = %d, dm = %u", close.w, close.h, close.format);	

	// Scale will be set by the width. Width will be changed to be best possible
	// Height will be set by scale.
	// Both width and height will be made even.

	// iPad 1/2
	// 1024/320 = 3.2 = 3; 1024/3 = 341.3 = 341 = 340
	// 768/3 = 256

	// iPad 3/4
	// 2048/320 = 6.4 = 6; 2048/6 = 341.3 = 341 = 340
	// 1536/6 = 256

	// iPhone 4/4s
	// 960/320 = 3; 960 / 3 = 320
	// 640/3 = 213.3 = 213 = 212

	// iPhone 5
	// 1136/320 = 3.55 = 3; 1136/3 = 378.6 = 378
	// 640/3 = 213.3 = 213 = 212

	// TODO something with former versions of iPhone. 
	// Scale factor must be 1.5 on them.

	int wf = close.w / 320;
	Graphics::SCREEN_WIDTH = int((close.w / wf) & 0xfffffffe);
	Graphics::SCREEN_HEIGHT = int((close.h / wf) & 0xfffffffe);

	NXSurface::SetScale(wf);
	
	return false;
}

bool Graphics::InitVideo()
{
	if (drawtarget == screen) drawtarget = NULL;
	if (screen) delete screen;
	
	uint32_t window_flags = SDL_WINDOW_SHOWN;
	if (is_fullscreen) window_flags |= SDL_WINDOW_FULLSCREEN;
	
	if (window)
	{
		stat("second call to Graphics::InitVideo()");
		abort();
	}
	
	stat("SDL_CreateWindow: %dx%d @ %dbpp", Graphics::SCREEN_WIDTH*SCALE, Graphics::SCREEN_HEIGHT*SCALE, screen_bpp);
	window = SDL_CreateWindow("NXEngine", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		Graphics::SCREEN_WIDTH*SCALE, Graphics::SCREEN_HEIGHT*SCALE,
		window_flags);

	if (!window)
	{
		staterr("Graphics::InitVideo: error setting video mode (SDL_CreateWindow: %s)", SDL_GetError());
		return 1;
	}

	int drv_index = -1;
#if 0
	{
		int drivers = SDL_GetNumRenderDrivers();
		SDL_RendererInfo info;
		for (int i = 0; i < drivers; ++i)
		{
			if (SDL_GetRenderDriverInfo(i, &info))
			{
				staterr("Graphics::InitVideo: SDL_GetRenderDriverInfo() failed: %s", SDL_GetError());
			}

			if (strcmp("opengl", info.name) == 0)
			{
				drv_index = i;
				break;
			}
		}
	}
#endif
	
	renderer = SDL_CreateRenderer(window, drv_index, /*SDL_RENDERER_SOFTWARE | */SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if (!renderer)
	{
		staterr("Graphics::InitVideo: error setting video mode (SDL_CreateRenderer: %s)", SDL_GetError());
		return 1;
	}

	SDL_RendererInfo info;
	if (SDL_GetRendererInfo(renderer, &info))
	{
		staterr("Graphics::InitVideo: SDL_GetRendererInfo failed: %s", SDL_GetError());
		return 1;
	}

	if (!(info.flags & SDL_RENDERER_TARGETTEXTURE))
	{
		staterr("Graphics::InitVideo: SDL_RENDERER_TARGETTEXTURE is not supported");
		return 1;
	}

	
	SDL_ShowCursor(is_fullscreen == false);

	screen = NXSurface::createScreen(Graphics::SCREEN_WIDTH*SCALE, Graphics::SCREEN_HEIGHT*SCALE, 
		info.texture_formats[0]);
    
    if (!screen)
    {
        staterr("Graphics::InitVideo: no screen has been created");
        return 1;
    }

	if (!drawtarget) drawtarget = screen;
	return 0;
}

bool Graphics::FlushAll()
{
	stat("Graphics::FlushAll()");
	palette_reset();
	Sprites::FlushSheets();
	Tileset::Reload();
	map_flush_graphics();
	return font_reload();
}

void Graphics::SetFullscreen(bool enable)
{
	if (is_fullscreen != enable)
	{
		is_fullscreen = enable;
		InitVideo();
		Graphics::FlushAll();
	}
}

// change the video mode to one of the available resolution codes, currently:
// 0 - 640x480, Fullscreen
// 1 - Windowed scale x1 (320x240)
// 2 - Windowed scale x2 (640x480)
// 3 - Windowed scale x3 (960x720)
bool Graphics::SetResolution(int r, bool restoreOnFailure)
{
	stat("Graphics::SetResolution(%d)", r);
	if (r == current_res)
		return 0;
	
	int old_res = current_res;

#ifdef IPHONE
	
	restoreOnFailure = false;
	
	if (SelectResolution())
	{
		staterr("SelectResolution() failed!");
		return 1;
	}

#else

	int factor;
	
	if (r == 0)
	{
		is_fullscreen = true;
		factor = 2;
	}
	else
	{
		is_fullscreen = false;
		factor = r;
	}
	
	stat("Setting scaling %d and fullscreen=%s", factor, is_fullscreen ? "yes":"no");
	NXSurface::SetScale(factor);
#endif

	if (Graphics::InitVideo())
	{
		staterr("Switch to resolution %d failed!", r);
		
		if (restoreOnFailure)
		{
			staterr("Trying to recover old mode %d.", r, old_res);
			if (Graphics::SetResolution(old_res, false))
			{
				staterr("Fatal error: vidmode recovery failed!!!");
			}
		}
		
		return 1;
	}
	
	if (Graphics::FlushAll()) return 1;
	return 0;
}

// return a pointer to a null-terminated list of available resolutions.
const char **Graphics::GetResolutions()
{
static const char *res_str[]   =
{
	"Fullscreen",
	"320x240", "640x480", "960x720",
	NULL
};

	return res_str;
}

/*
void c------------------------------() {}
*/

// copy a sprite into the current tileset.
// used to obtain the images for the star tiles (breakable blocks),
// and for animated motion tiles in Waterway.
void Graphics::CopySpriteToTile(int spr, int tileno, int offset_x, int offset_y)
{
NXRect srcrect, dstrect;

	srcrect.x = (sprites[spr].frame[0].dir[0].sheet_offset.x + offset_x);
	srcrect.y = (sprites[spr].frame[0].dir[0].sheet_offset.y + offset_y);
	srcrect.w = TILE_W;
	srcrect.h = TILE_H;
	
	dstrect.x = (tileno % 16) * TILE_W;
	dstrect.y = (tileno / 16) * TILE_H;
	dstrect.w = TILE_W;
	dstrect.h = TILE_H;
	
	NXSurface *tileset = Tileset::GetSurface();
	NXSurface *spritesheet = Sprites::get_spritesheet(sprites[spr].spritesheet);
	
	if (tileset && spritesheet)
	{
		// blank out the old tile data with clear
		tileset->ClearRect(&dstrect);
		
		// copy the sprite over
		BlitSurface(spritesheet, &srcrect, tileset, &dstrect);
	}
}


void Graphics::ShowLoadingScreen()
{
NXSurface loading;
char fname[MAXPATHLEN];
	
	sprintf(fname, "%s/Loading.pbm", data_dir);
	if (loading.LoadImage(fname))
		return;
	
	int x = (Graphics::SCREEN_WIDTH / 2) - (loading.Width() / 2);
	int y = (Graphics::SCREEN_HEIGHT / 2) - loading.Height();
	
	ClearScreen(BLACK);
	DrawSurface(&loading, x, y);
	drawtarget->Flip();
}

/*
void c------------------------------() {}
*/

// blit from one surface to another, just like SDL_BlitSurface.
void Graphics::BlitSurface(NXSurface *src, NXRect *srcrect, NXSurface *dst, NXRect *dstrect)
{
	dst->DrawSurface(src, dstrect->x, dstrect->y, \
					 srcrect->x, srcrect->y, srcrect->w, srcrect->h);
}

/*
void c------------------------------() {}
*/

// draw the entire surface to the screen at the given coordinates.
void Graphics::DrawSurface(NXSurface *src, int x, int y)
{
	drawtarget->DrawSurface(src, x, y);
}


// blit the specified portion of the surface to the screen
void Graphics::DrawSurface(NXSurface *src, \
						   int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
	drawtarget->DrawSurface(src, dstx, dsty, srcx, srcy, wd, ht);
}


// blit the specified surface across the screen in a repeating pattern
void Graphics::BlitPatternAcross(NXSurface *sfc, int x_dst, int y_dst, int y_src, int height)
{
	drawtarget->BlitPatternAcross(sfc, x_dst, y_dst, y_src, height);
}


void Graphics::DrawBatchBegin(size_t max_count)
{
	if (NULL != current_batch_drawtarget)
		assert(false && "batch operation already begun");

	current_batch_drawtarget = drawtarget;

	drawtarget->DrawBatchBegin(max_count);
}

void Graphics::DrawBatchAdd(NXSurface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
	if (current_batch_drawtarget != drawtarget)
		assert(false && "drawtarget has been changed during batch operation");

	drawtarget->DrawBatchAdd(src, dstx, dsty, srcx, srcy, wd, ht);
}

void Graphics::DrawBatchAdd(NXSurface *src, int x, int y)
{
	if (current_batch_drawtarget != drawtarget)
		assert(false && "drawtarget has been changed during batch operation");

	drawtarget->DrawBatchAdd(src, x, y);
}

void Graphics::DrawBatchAddPatternAcross(NXSurface *sfc, int x_dst, int y_dst, int y_src, int height)
{
    if (current_batch_drawtarget != drawtarget)
		assert(false && "drawtarget has been changed during batch operation");
    
	drawtarget->DrawBatchAddPatternAcross(sfc, x_dst, y_dst, y_src, height);
}

void Graphics::DrawBatchEnd()
{
	if (current_batch_drawtarget != drawtarget)
		assert(false && "drawtarget has been changed during batch operation");

	current_batch_drawtarget = NULL;

	drawtarget->DrawBatchEnd();
}


/*
void c------------------------------() {}
*/

void Graphics::DrawLine(int x1, int y1, int x2, int y2, NXColor color)
{
	drawtarget->DrawLine(x1, y1, x2, y2, color);
}

void Graphics::DrawRect(int x1, int y1, int x2, int y2, NXColor color)
{
	drawtarget->DrawRect(x1, y1, x2, y2, color);
}

void Graphics::FillRect(int x1, int y1, int x2, int y2, NXColor color)
{
	drawtarget->FillRect(x1, y1, x2, y2, color);
}

void Graphics::DrawPixel(int x, int y, NXColor color)
{
	drawtarget->DrawPixel(x, y, color);
}

void Graphics::ClearScreen(NXColor color)
{
	drawtarget->Clear(color.r, color.g, color.b);
}

/*
void c------------------------------() {}
*/

void Graphics::DrawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
	drawtarget->DrawRect(x1, y1, x2, y2, r, g, b);
}

void Graphics::FillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
	drawtarget->FillRect(x1, y1, x2, y2, r, g, b);
}

void Graphics::DrawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	drawtarget->DrawPixel(x, y, r, g, b);
}

void Graphics::ClearScreen(uint8_t r, uint8_t g, uint8_t b)
{
	drawtarget->Clear(r, g, b);
}

/*
void c------------------------------() {}
*/

void Graphics::set_clip_rect(int x, int y, int w, int h)
{
	drawtarget->set_clip_rect(x, y, w, h);
}

void Graphics::set_clip_rect(NXRect *rect)
{
	drawtarget->set_clip_rect(rect);
}

void Graphics::clear_clip_rect()
{
	drawtarget->clear_clip_rect();
}

bool Graphics::is_set_clip()
{
	return drawtarget->is_set_clip();
}

void Graphics::clip(SDL_Rect& srcrect, SDL_Rect& dstrect)
{
	drawtarget->clip(srcrect, dstrect);
}

/*
void c------------------------------() {}
*/

// change the target surface of operation like DrawRect to something
// other than the screen.
void Graphics::SetDrawTarget(NXSurface *surface)
{
	surface->SetAsTarget(surface != screen);
	drawtarget = surface;
}








