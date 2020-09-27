#include <algorithm>
#include <cassert>
// graphics routines
#include "../ResourceManager.h"
#include "../config.h"
#include "../game.h"
#include "../map.h"
#include "../nx.h"
#include "../pause/dialog.h"
#include "../version.h"
#include "Renderer.h"
#include "nx_icon.h"
#include "pngfuncs.h"
#include "../Utils/Logger.h"

#include <SDL.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#if defined(__VITA__)
#include <vita2d.h>
#endif

namespace NXE
{
namespace Graphics
{

Renderer::Renderer() {}
Renderer::~Renderer() {}

Renderer *Renderer::getInstance()
{
  return Singleton<Renderer>::get();
}

bool Renderer::init(int resolution)
{
  _current_res = resolution;
  if (!initVideo())
    return false;

  font.load(std::string("font_" + std::to_string(scale) + ".fnt"));

  if (!sprites.init())
    return false;

  return true;
}

void Renderer::close()
{
  LOG_INFO("Renderer::Close()");
  font.cleanup();
  sprites.close();
  SDL_ShowCursor(true);
  SDL_DestroyWindow(_window);
  _window = NULL;
}

bool Renderer::isWindowVisible()
{
  Uint32 flags = SDL_GetWindowFlags(_window);

  return (flags & SDL_WINDOW_SHOWN) && !(flags & SDL_WINDOW_MINIMIZED) // SDL_APPACTIVE
         && (flags & SDL_WINDOW_INPUT_FOCUS);                          // SDL_APPINPUTFOCUS
}

bool Renderer::initVideo()
{
  uint32_t window_flags = SDL_WINDOW_SHOWN;

  const NXE::Graphics::gres_t *res = getResolutions();

  uint32_t width  = res[_current_res].width;
  uint32_t height = res[_current_res].height;
  scale        = res[_current_res].scale;
  screenHeight = res[_current_res].base_height;
  screenWidth  = res[_current_res].base_width;
  widescreen   = res[_current_res].widescreen;

  if (_window)
  {
    LOG_WARN("second call to Renderer::InitVideo()");
    return false;
  }

  LOG_DEBUG("SDL_CreateWindow: {}x{}", width, height);
  _window = SDL_CreateWindow(NXVERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);

  if (!_window)
  {
    LOG_ERROR("Renderer::initVideo: error setting video mode (SDL_CreateWindow: {})", SDL_GetError());
    return false;
  }

#if not defined(__VITA__) && not defined(__SWITCH__)
  SDL_Surface *icon;
  icon = SDL_CreateRGBSurfaceFrom((void *)WINDOW_TITLE_ICON.pixel_data, WINDOW_TITLE_ICON.width,
                                  WINDOW_TITLE_ICON.height, WINDOW_TITLE_ICON.bytes_per_pixel * 8,
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
  SDL_SetWindowIcon(_window, icon);
  SDL_FreeSurface(icon);
#endif

  if (!_renderer)
  {
#if defined(__VITA__)
    vita2d_init_advanced(2 * 1024 * 1024);
#endif
    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
  }
  if (!_renderer)
  {
    LOG_ERROR("Renderer::initVideo: error setting video mode (SDL_CreateRenderer: {})", SDL_GetError());
    return false;
  }

  SDL_RendererInfo info;
  if (SDL_GetRendererInfo(_renderer, &info))
  {
    LOG_ERROR("Renderer::initVideo: SDL_GetRendererInfo failed: {}", SDL_GetError());
    return false;
  }

  LOG_INFO("Renderer::initVideo: using: {} renderer", info.name);
  return true;
}

bool Renderer::flushAll()
{
  LOG_DEBUG("Renderer::flushAll()");
  SDL_RenderPresent(_renderer);
  sprites.flushSheets();
  tileset.reload();
  map_flush_graphics();
  font.cleanup();
  font.load(std::string("font_" + std::to_string(scale) + ".fnt"));
  return true;
}

void Renderer::setFullscreen(bool enable)
{
  SDL_ShowCursor(!enable);
  SDL_SetWindowFullscreen(_window, (enable ? SDL_WINDOW_FULLSCREEN : 0));
}

bool Renderer::setResolution(int r, bool restoreOnFailure)
{
#if defined(__VITA__) || defined(__SWITCH__)
  r = 1; // one fixed resolution
#endif

  LOG_INFO("Renderer::setResolution({})", r);
  if (r == _current_res)
    return 0;

  uint32_t width  = screenWidth;
  uint32_t height = screenHeight;

  if (r == 0)
  {
    scale = 1;
    widescreen = false;
  }
  else
  {
    const NXE::Graphics::gres_t *res = getResolutions();
    scale        = res[r].scale;
    screenHeight = res[r].base_height;
    screenWidth  = res[r].base_width;
    widescreen   = res[r].widescreen;
    width        = res[r].width;
    height       = res[r].height;
  }

  LOG_INFO("Setting scaling {}", scale);

  SDL_SetWindowSize(_window, width, height);

  _current_res = r;

  if (!flushAll())
    return false;

  recalc_map_offsets();
  textbox.RecalculateOffsets();

  return true;
}

const Graphics::gres_t *Renderer::getResolutions()
{
  static NXE::Graphics::gres_t res[]
      = {//      description, screen_w, screen_h, render_w, render_h, scale_factor, widescreen, enabled
         // 4:3
         {(char *)"---", 0, 0, 0, 0, 1, false, true},
#if defined(__VITA__)
         {(char *)"960x544", 960, 544, 480, 272, 2, true, true},
#elif defined(__SWITCH__)
         {(char *)"1920x1080", 1920, 1080, 480, 270, 4, true, true},
#else
         {(char *)"320x240", 320, 240, 320, 240, 1, false, true},
         {(char *)"640x480", 640, 480, 320, 240, 2, false, true},
         //        {(char*)"800x600",   800,      600,      320,      240,      2.5,          false,      true },
         //        //requires float scalefactor
         {(char *)"1024x768", 1024, 768, 341, 256, 3, false, true},
         {(char *)"1280x1024", 1280, 1024, 320, 256, 4, false, true},
         {(char *)"1600x1200", 1600, 1200, 320, 240, 5, false, true},
         // widescreen
         {(char *)"480x272", 480, 272, 480, 272, 1, true, true},
         {(char *)"1360x768", 1360, 768, 454, 256, 3, true, true},
         {(char *)"1366x768", 1366, 768, 455, 256, 3, true, true},
         {(char *)"1440x900", 1440, 900, 480, 300, 3, true, true},
         {(char *)"1600x900", 1600, 900, 533, 300, 3, true, true},
         {(char *)"1920x1080", 1920, 1080, 480, 270, 4, true, true},
#endif
         {NULL, 0, 0, 0, 0, 0, false, false}};

  SDL_DisplayMode dm;
  SDL_GetDesktopDisplayMode(0, &dm);

  LOG_DEBUG("DW: {}, DH: {}", dm.w, dm.h);
  for (int i = 0; res[i].name; i++)
  {
    if (res[i].width > (uint32_t)dm.w || res[i].height > (uint32_t)dm.h)
    {
      LOG_INFO("Disabling {}", res[i].name);

      res[i].enabled = false;
    }
  }

  return res;
}

int Renderer::getResolutionCount()
{
  int i;
  const gres_t *res = getResolutions();

  for (i = 0; res[i].name; i++)
    ;
  return i;
}

void Renderer::showLoadingScreen()
{
  Surface loading;

  if (!loading.loadImage(ResourceManager::getInstance()->getLocalizedPath("Loading.pbm")))
    return;

  int x = (screenWidth / 2) - (loading.width() / 2);
  int y = (screenHeight / 2) - loading.height();

  clearScreen(BLACK);
  drawSurface(&loading, x, y);
  flip();
}

SDL_Renderer* Renderer::renderer()
{
  return _renderer;
}

SDL_Window* Renderer::window()
{
  return _window;
}

void Renderer::flip()
{
  SDL_RenderPresent(_renderer);
}

// blit the specified portion of the surface to the screen
void Renderer::drawSurface(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
  assert(_renderer);
  assert(src->texture());

  SDL_Rect srcrect, dstrect;

  srcrect.x = srcx * scale;
  srcrect.y = srcy * scale;
  srcrect.w = wd * scale;
  srcrect.h = ht * scale;

  dstrect.x = dstx * scale;
  dstrect.y = dsty * scale;
  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;

  if (_need_clip)
    clipScaled(srcrect, dstrect);

  SDL_SetTextureAlphaMod(src->texture(), src->alpha);
  if (SDL_RenderCopy(_renderer, src->texture(), &srcrect, &dstrect))
  {
    LOG_ERROR("Renderer::drawSurface: SDL_RenderCopy failed: {}", SDL_GetError());
  }
}

// blit the specified portion of the surface to the screen
void Renderer::drawSurfaceMirrored(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
  assert(_renderer);
  assert(src->texture());

  SDL_Rect srcrect, dstrect;

  srcrect.x = srcx * scale;
  srcrect.y = srcy * scale;
  srcrect.w = wd * scale;
  srcrect.h = ht * scale;

  dstrect.x = dstx * scale;
  dstrect.y = dsty * scale;
  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;

  if (_need_clip)
    clipScaled(srcrect, dstrect);

  SDL_SetTextureAlphaMod(src->texture(), src->alpha);
  if (SDL_RenderCopyEx(_renderer, src->texture(), &srcrect, &dstrect, 0, NULL, SDL_FLIP_HORIZONTAL))
  {
    LOG_ERROR("Renderer::drawSurface: SDL_RenderCopy failed: {}", SDL_GetError());
  }
}

// blit the specified surface across the screen in a repeating pattern
void Renderer::blitPatternAcross(Surface *sfc, int x_dst, int y_dst, int y_src, int height)
{
  SDL_Rect srcrect, dstrect;

  srcrect.x = 0;
  srcrect.w = sfc->width() * scale;
  srcrect.y = (y_src * scale);
  srcrect.h = (height * scale);

  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;

  int x      = (x_dst * scale);
  int y      = (y_dst * scale);
  int destwd = screenWidth * scale;

  assert(!_need_clip && "clip for blitpattern is not implemented");

  do
  {
    dstrect.x = x;
    dstrect.y = y;
    SDL_RenderCopy(_renderer, sfc->texture(), &srcrect, &dstrect);
    x += sfc->width()  * scale;
  } while (x < destwd);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, NXColor color)
{
  SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
  SDL_RenderDrawLine(_renderer, x1 * scale, y1 * scale, x2 * scale, y2 * scale);
}

void Renderer::drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  SDL_Rect rects[4] = {{x1 * scale, y1 * scale, ((x2 - x1) + 1) * scale, scale},
  {x1 * scale, y2 * scale, ((x2 - x1) + 1) * scale, scale},
  {x1 * scale, y1 * scale, scale, ((y2 - y1) + 1) * scale},
  {x2 * scale, y1 * scale, scale, ((y2 - y1) + 1) * scale}};

  SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRects(_renderer, rects, 4);
}

void Renderer::fillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  SDL_Rect rect;

  rect.x = x1 * scale;
  rect.y = y1 * scale;
  rect.w = ((x2 - x1) + 1) * scale;
  rect.h = ((y2 - y1) + 1) * scale;

  SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(_renderer, &rect);}

void Renderer::tintScreen()
{
  SDL_SetRenderDrawBlendMode(_renderer,SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 150);
  SDL_RenderFillRect(_renderer, NULL);
}

void Renderer::drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  fillRect(x, y, x, y, r, g, b);
}

void Renderer::clearScreen(uint8_t r, uint8_t g, uint8_t b)
{
  SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(_renderer, NULL);
}

void Renderer::setClip(int x, int y, int w, int h)
{
  _need_clip = true;

  _clip_rect.x = x * scale;
  _clip_rect.y = y * scale;
  _clip_rect.w = w * scale;
  _clip_rect.h = h * scale;
}

void Renderer::clearClip()
{
  _need_clip = false;
}

bool Renderer::isClipSet()
{
  return _need_clip;
}

void Renderer::clip(SDL_Rect &srcrect, SDL_Rect &dstrect)
{
  int w,h;
  int dx, dy;

  w = dstrect.w;
  h = dstrect.h;

  dx = _clip_rect.x - dstrect.x;
  if (dx > 0)
  {
    w -= dx;
    dstrect.x += dx;
    srcrect.x += dx / scale;
  }
  dx = dstrect.x + w - _clip_rect.x - _clip_rect.w;
  if (dx > 0)
    w -= dx;

  dy = _clip_rect.y - dstrect.y;
  if (dy > 0)
  {
    h -= dy;
    dstrect.y += dy;
    srcrect.y += dy / scale;
  }
  dy = dstrect.y + h - _clip_rect.y - _clip_rect.h;
  if (dy > 0)
    h -= dy;

  dstrect.w = w;
  dstrect.h = h;
  srcrect.w = w / scale;
  srcrect.h = h / scale;
}

void Renderer::clipScaled(SDL_Rect &srcrect, SDL_Rect &dstrect)
{
  int w, h;
  int dx, dy;

  w = dstrect.w;
  h = dstrect.h;

  dx = _clip_rect.x - dstrect.x;
  if (dx > 0)
  {
    w -= dx;
    dstrect.x += dx;
    srcrect.x += dx;
  }
  dx = dstrect.x + w - _clip_rect.x - _clip_rect.w;
  if (dx > 0)
    w -= dx;

  dy = _clip_rect.y - dstrect.y;
  if (dy > 0)
  {
    h -= dy;
    dstrect.y += dy;
    srcrect.y += dy;
  }
  dy = dstrect.y + h - _clip_rect.y - _clip_rect.h;
  if (dy > 0)
    h -= dy;

  dstrect.w = srcrect.w = w;
  dstrect.h = srcrect.h = h;
}

void Renderer::saveScreenshot()
{
  auto filename = []()
  {
    int iter = 0;
    std::string name;
    do
    {
      std::ostringstream oss;
      oss << "screenshot";
      oss << std::setfill('0') << std::setw(3);
      oss << ++iter;
      oss << ".png";
      name = ResourceManager::getInstance()->getPrefPath(oss.str());
    } while (ResourceManager::getInstance()->fileExists(name) && iter < 1000);
    if (ResourceManager::getInstance()->fileExists(name))
    {
        return std::string();
    }
    return name;
  }();

  if (filename.empty())
  {
    LOG_ERROR("Can not get screenshot name. Too many screenshots in folder");
    return;
  }

  SDL_Rect viewport;
  SDL_RenderGetViewport(_renderer, &viewport);
  SDL_Surface* surface = SDL_CreateRGBSurface(0, viewport.w, viewport.h, 24,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
                                 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
  );

  if (!surface)
  {
    LOG_ERROR("Couldn't create surface: {}", SDL_GetError());
    return;
  }

  if (SDL_RenderReadPixels(_renderer, NULL, surface->format->format, surface->pixels, surface->pitch) < 0)
  {
    LOG_ERROR("Couldn't read screen: {}", SDL_GetError());
    return;
  }

  if (png_save_surface(filename, surface) < 0)
  {
    SDL_FreeSurface(surface);
    LOG_ERROR("Couldn't save screen");
    return;
  }
  SDL_FreeSurface(surface);
  LOG_INFO("Saved {}", filename);
  return;
}

};
};
