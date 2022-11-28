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
#include <SDL_image.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>

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

bool Renderer::init(int scale, bool newWidescreen)
{
  widescreen = newWidescreen;
  if (!initVideo(scale))
    return false;

  if (!font.load())
    return false;

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

bool Renderer::initVideo(int scale)
{
  uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

  //TODO: sync this with setResolution
  if (widescreen) {
    screenWidth  = 432;
    screenHeight = 243;
  } else {
    screenWidth  = 320;
    screenHeight = 240;
  }

  if (_window)
  {
    LOG_WARN("second call to Renderer::InitVideo()");
    return false;
  }

  LOG_DEBUG("SDL_CreateWindow: {}x{}", screenWidth, screenHeight);
  _window = SDL_CreateWindow(NXVERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth * scale, screenHeight * scale, window_flags);

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

  if (SDL_RenderSetLogicalSize(_renderer, screenWidth, screenHeight))
  {
    LOG_ERROR("Renderer::initVideo: SDL_RenderSetLogicalSize failed: {}", SDL_GetError());
    return false;
  }

  if (!createRenderTarget(screenWidth, screenHeight))
    return false;

  std::string spotpath = ResourceManager::getInstance()->getPath("spot.png");

  SDL_Surface *image;
  image = IMG_Load(spotpath.c_str());
  _spot_light = SDL_CreateTextureFromSurface(_renderer, image);
  SDL_FreeSurface(image);

  return true;
}

bool Renderer::createRenderTarget(int width, int height)
{
  SDL_RendererInfo info;
  SDL_GetRendererInfo(_renderer, &info);

  _texture = SDL_CreateTexture(_renderer,
      info.texture_formats[0],
      SDL_TEXTUREACCESS_TARGET,
      width, height);
  if (SDL_SetRenderTarget(_renderer, _texture)) {
    LOG_ERROR("Renderer::createRenderTarget: SDL_SetRenderTarget failed: {}", SDL_GetError());
    return false;
  }
  SDL_RenderClear(_renderer);

  return true;
}

bool Renderer::flushAll()
{
  LOG_DEBUG("Renderer::flushAll()");
  SDL_RenderPresent(_renderer);
  sprites.flushSheets();
  tileset.reload();
  map_flush_graphics();
  if (!font.load())
    return false;

  return true;
}

bool Renderer::setFullscreen(bool enable)
{
  _fullscreen = enable;
  SDL_ShowCursor(!enable);
  if (SDL_SetWindowFullscreen(_window, (enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))) {
    LOG_ERROR("Renderer::setFullscreen: SDL_SetWindowFullscreen failed: {}", SDL_GetError());
    return false;
  }
  if (SDL_RenderSetLogicalSize(_renderer, screenWidth, screenHeight)) {
    LOG_ERROR("Renderer::setFullscreen: SDL_RenderSetLogicalSize failed: {}", SDL_GetError());
    return false;
  }

  return true;
}

bool Renderer::setResolution(int scale, bool newWidescreen)
{
  int newWidth, newHeight;
#if defined(__VITA__)
  newWidth = 480;
  newHeight = 272;
  newWidescreen = true;
#else
#if defined(__SWITCH__)
  newWidescreen = true;
#endif
  if (newWidescreen) {
    newWidth  = 432;
    newHeight = 243;
  } else {
    newWidth  = 320;
    newHeight = 240;
  }
#endif

  LOG_INFO("Renderer logical resolution: {}x{}", newWidth, newHeight);

  SDL_SetWindowSize(_window, newWidth * scale, newHeight * scale);

  SDL_SetRenderTarget(_renderer, NULL);
  SDL_DestroyTexture(_texture);
  if (SDL_RenderSetLogicalSize(_renderer, newWidth, newHeight)) {
    LOG_ERROR("Renderer::setResolution: SDL_RenderSetLogicalSize failed: {}", SDL_GetError());
    return false;
  }

  if (!flushAll())
    return false;

  if (!createRenderTarget(newWidth, newHeight))
    return false;

  screenWidth = newWidth;
  screenHeight = newHeight;
  widescreen = newWidescreen;

  recalc_map_offsets();
  textbox.RecalculateOffsets();

  return true;
}

int Renderer::getResolutionCount()
{
#if defined(__VITA__) || defined(__SWITCH__)
  return 1;
#else
  return 4;
#endif
}

void Renderer::showLoadingScreen()
{
  Surface loading;

  if (!loading.loadImage(ResourceManager::getInstance()->getPath("Loading.pbm")))
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
//  LOG_INFO("===FLIPPING===\n");
  SDL_SetRenderTarget(_renderer, NULL);
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(_renderer);
  SDL_RenderCopy(_renderer, _texture, NULL, NULL);
  SDL_RenderPresent(_renderer);
  SDL_SetRenderTarget(_renderer, _texture);
//  LOG_INFO("===FLIPPED===\n");
}

// blit the specified portion of the surface to the screen
void Renderer::drawSurface(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
  assert(_renderer);
  assert(src->texture());

  SDL_Rect srcrect, dstrect;

  srcrect.x = srcx;
  srcrect.y = srcy;
  srcrect.w = wd;
  srcrect.h = ht;

  dstrect.x = dstx;
  dstrect.y = dsty;
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

  srcrect.x = srcx;
  srcrect.y = srcy;
  srcrect.w = wd;
  srcrect.h = ht;

  dstrect.x = dstx;
  dstrect.y = dsty;
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
  srcrect.w = sfc->width();
  srcrect.y = y_src;
  srcrect.h = height;

  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;

  int x      = x_dst;
  int y      = y_dst;
  int destwd = screenWidth;

  assert(!_need_clip && "clip for blitpattern is not implemented");

  do
  {
    dstrect.x = x;
    dstrect.y = y;
    SDL_RenderCopy(_renderer, sfc->texture(), &srcrect, &dstrect);
    x += sfc->width();
  } while (x < destwd);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, NXColor color)
{
  SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
  SDL_RenderDrawLine(_renderer, x1, y1, x2, y2);
}

void Renderer::drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  SDL_Rect rects[4] = {{x1, y1, ((x2 - x1) + 1), 1},
  {x1, y2, ((x2 - x1) + 1), 1},
  {x1, y1, 1, ((y2 - y1) + 1)},
  {x2, y1, 1, ((y2 - y1) + 1)}};

  SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRects(_renderer, rects, 4);
}

void Renderer::fillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  SDL_Rect rect;

  rect.x = x1;
  rect.y = y1;
  rect.w = ((x2 - x1) + 1);
  rect.h = ((y2 - y1) + 1);

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

  _clip_rect.x = x;
  _clip_rect.y = y;
  _clip_rect.w = w;
  _clip_rect.h = h;
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

  dstrect.w = w;
  dstrect.h = h;
  srcrect.w = w;
  srcrect.h = h;
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

void Renderer::drawSpotLight(int x, int y, Object* o, int r, int g, int b, int upscale)
{
  SDL_Rect dstrec;
  int width = o->Width() / CSFI;
  int height = o->Height() / CSFI;

  dstrec.x = (x - (width * (upscale / 2)) + (width / 2));
  dstrec.y = (y - (height * (upscale / 2)) + (height / 2));
  dstrec.w = width * upscale;
  dstrec.h = height * upscale;

  SDL_SetTextureColorMod(_spot_light, r, g, b);
  SDL_RenderCopy(_renderer, _spot_light, NULL, &dstrec);
}



};
};
