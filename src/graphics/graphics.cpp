#include <cassert>
#include <algorithm>
// graphics routines
#include <SDL.h>

#include <cstdlib>
#include "../nx.h"
#include "../config.h"
#include "graphics.h"
#include "tileset.h"
#include "sprites.h"
#include "font.h"
#include "../map.h"
#include "../version.h"
#include "../ResourceManager.h"
#include "nx_icon.h"
#include "../game.h"
#include "../pause/dialog.h"

SDL_Window * window = NULL;
SDL_Renderer * renderer = NULL;


// (unscaled) screen size/video mode
int Graphics::SCREEN_WIDTH = 320;
int Graphics::SCREEN_HEIGHT = 240;
bool Graphics::widescreen = false;

NXSurface *screen = NULL;				// created from SDL's screen
static NXSurface *drawtarget = NULL;	// target of DrawRect etc; almost always screen
bool use_palette = false;				// true if we are in an indexed-color video mode
int screen_bpp;

const NXColor DK_BLUE(0, 0, 0x21);		// the popular dk blue backdrop color
const NXColor BLACK(0, 0, 0);			// pure black, only works if no colorkey
//const NXColor CLEAR(0, 0, 0);			// the transparent/colorkey color


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
	}
	
	
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


bool Graphics::InitVideo()
{
	if (drawtarget == screen) drawtarget = NULL;
	if (screen) delete screen;
	
	uint32_t window_flags = SDL_WINDOW_SHOWN;
	
	const Graphics::gres_t* res=GetRes();
	
	uint32_t width = res[current_res].width;
	uint32_t height = res[current_res].height;
	
	if (window)
	{
		stat("second call to Graphics::InitVideo()");
	}
	
	stat("SDL_CreateWindow: %dx%d @ %dbpp", width, height, screen_bpp);
	if (window)
	{
        	SDL_SetWindowSize(window, width, height);
	}
	else
	{
    	window = SDL_CreateWindow(NXVERSION, 
	    	SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    		width, height,
    		window_flags);
	}

	if (!window)
	{
		staterr("Graphics::InitVideo: error setting video mode (SDL_CreateWindow: %s)", SDL_GetError());
		return 1;
	}

    SDL_Surface *icon;
    icon = SDL_CreateRGBSurfaceFrom((void *)WINDOW_TITLE_ICON.pixel_data,
                                    WINDOW_TITLE_ICON.width,
                                    WINDOW_TITLE_ICON.height,
                                    WINDOW_TITLE_ICON.bytes_per_pixel * 8,
                                    WINDOW_TITLE_ICON.bytes_per_pixel * WINDOW_TITLE_ICON.width,
                                #if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                                    0xff000000, /* Red bit mask. */
                                    0x00ff0000, /* Green bit mask. */
                                    0x0000ff00, /* Blue bit mask. */
                                    0x000000ff  /* Alpha bit mask. */
                                #else
                                    0x000000ff, /* Red bit mask. */
                                    0x0000ff00, /* Green bit mask. */
                                    0x00ff0000, /* Blue bit mask. */
                                    0xff000000  /* Alpha bit mask. */
                                #endif
                                    );
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);


	if (!renderer)
    	renderer = SDL_CreateRenderer(window, -1, /*SDL_RENDERER_SOFTWARE | */SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
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

	stat("Graphics::InitVideo: using: %s renderer", info.name);

	if (!(info.flags & SDL_RENDERER_TARGETTEXTURE))
	{
		staterr("Graphics::InitVideo: SDL_RENDERER_TARGETTEXTURE is not supported");
		return 1;
	}

	screen = NXSurface::createScreen(width, height, 
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
	SDL_RenderPresent(renderer);
	Sprites::FlushSheets();
	Tileset::Reload();
	map_flush_graphics();
	return font_reload();
}

void Graphics::SetFullscreen(bool enable)
{
	SDL_ShowCursor(!enable);
	SDL_SetWindowFullscreen(window, (enable ? SDL_WINDOW_FULLSCREEN : 0));
	Graphics::FlushAll();

}

// change the video mode to one of the available resolution codes, currently:
// 0 - 640x480, Fullscreen
// 1 - Windowed scale x1 (320x240)
// 2 - Windowed scale x2 (640x480)
// 3 - Windowed scale x3 (960x720)
extern std::vector<void*>  optionstack;

bool Graphics::SetResolution(int r, bool restoreOnFailure)
{
	stat("Graphics::SetResolution(%d)", r);
	if (r == current_res)
		return 0;
	
	int old_res = current_res;

	int factor;
	
	if (r == 0)
	{
		factor = 1;
	}
	else
	{
	    const Graphics::gres_t* res=GetRes();
		factor = res[r].scale;
		SCREEN_HEIGHT = res[r].base_height;
		SCREEN_WIDTH = res[r].base_width;
		widescreen = res[r].widescreen;
	}
	
	stat("Setting scaling %d", factor);
	NXSurface::SetScale(factor);
    current_res = r;
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
			current_res = old_res;
		}
		
		return 1;
	}
	current_res = r;
	if (Graphics::FlushAll()) return 1;
	recalc_map_offsets();
	textbox.RecalculateOffsets();
	for (auto dlg : optionstack)
	{
	    ((Options::Dialog*)dlg)->UpdateSizePos();
	}
	return 0;
}


const Graphics::gres_t* Graphics::GetRes()
{
    static Graphics::gres_t res[] = {
        //      description,     screen_w, screen_h, render_w, render_h, scale_factor, widescreen, enabled
        {(char*)"---",                  0,        0,        0,        0,            1,      false,    true },
        {(char*)"=320x240= (4:3)",    320,      240,      320,      240,            1,      false,    true },
        {(char*)"384x240 (16:10)",    384,      240,      384,      240,            1,       true,    true },
        {(char*)"480x272 (16:9)",     480,      272,      480,      272,            1,       true,    true },
        {(char*)"=640x480= (4:3)",    640,      480,      320,      240,            2,      false,    true },
        {(char*)"720x576 (5:4)",      720,      576,      360,      288,            2,      false,    true },
        {(char*)"768x480 (16:10)",    768,      480,      384,      240,            2,       true,    true },
        {(char*)"800x600 (4:3)",      800,      600,      400,      300,            2,      false,    true }, // ideally scalefactor 2.5
        {(char*)"=960x720= (4:3)",    960,      720,      320,      240,            3,      false,    true },
        {(char*)"1024x768 (4:3)",    1024,      768,      341,      256,            3,      false,    true },
        {(char*)"1152x720 (16:10)",  1152,      720,      384,      240,            3,       true,    true },
        {(char*)"1280x720 (16:9)",   1280,      720,      427,      240,            3,       true,    true },
        {(char*)"1280x800 (16:10)",  1280,      800,      427,      267,            3,       true,    true },
        {(char*)"=1280x960= (4:3)",  1280,      960,      320,      240,            4,      false,    true },
        {(char*)"1280x1024 (5:4)",   1280,     1024,      320,      256,            4,      false,    true },
        {(char*)"1360x768 (16:9)",   1360,      768,      454,      256,            3,       true,    true },
        {(char*)"1366x768 (16:9)",   1366,      768,      455,      256,            3,       true,    true },
        {(char*)"1440x900 (16:10)",  1440,      900,      480,      300,            3,       true,    true },
        {(char*)"1536x960 (16:10)",  1536,      960,      384,      240,            4,       true,    true },
        {(char*)"1600x900 (16:9)",   1600,      900,      534,      300,            3,       true,    true },
        {(char*)"=1600x1200= (4:3)", 1600,     1200,      320,      240,            5,      false,    true },
        {(char*)"1680x1050 (16:10)", 1680,     1050,      420,      263,            4,       true,    true },
        {(char*)"1920x1080 (16:9)",  1920,     1080,      480,      270,            4,       true,    true },
        {(char*)"1920x1200 (16:10)", 1920,     1200,      384,      240,            5,       true,    true },
        {NULL, 0, 0, 0, 0, 0, false, false}
    };

    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);

    stat("DW: %d, DH: %d",dm.w, dm.h);
    for (int i=0;res[i].name;i++)
    {
        if (res[i].width>(uint32_t)dm.w || res[i].height>(uint32_t)dm.h)
        {
            stat("Disabling %s",res[i].name);

            res[i].enabled=false;
        }
    }

    return res;
}

int Graphics::GetResCount()
{
  int i;
  const gres_t *res = GetRes();

  for (i=0;res[i].name;i++);
  return i;
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

	if (loading.LoadImage(ResourceManager::getInstance()->getLocalizedPath("Loading.pbm")))
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


