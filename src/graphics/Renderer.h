#ifndef _RENDERER_H
#define _RENDERER_H

#include "../Singleton.h"
#include "../object.h"
#include "Surface.h"
#include "Font.h"
#include "Tileset.h"
#include "Sprites.h"

#include <SDL.h>


namespace NXE
{
namespace Graphics
{

const NXColor DK_BLUE(0,0,0x21);
const NXColor BLACK(0,0,0);
const NXColor WHITE(0xFF,0xFF,0xFF);
const NXColor GREEN1(0x0,0xFF,0x0);
const NXColor GREEN2(0x0,0x98,0x0);
const NXColor GREEN3(0x0,0x4E,0x0);
const NXColor GREEN4(0x0,0x19,0x0);

class Renderer
{
  public:
    static Renderer *getInstance();

    int screenWidth = 320;
    int screenHeight = 240;
    bool widescreen = false;

    bool init(int scale, bool newWidescreen);
    void close();

    bool isWindowVisible();

    bool initVideo(int scale);
    bool setFullscreen(bool enable);

    bool setResolution(int factor, bool restoreOnFailure = true);
    int getResolutionCount();

    bool flushAll();

    void showLoadingScreen();

    void drawSurface(Surface *src, int x, int y);
    void drawSurface(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);
    void drawSurfaceMirrored(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht);

    void blitPatternAcross(Surface *sfc, int x_dst, int y_dst, int y_src, int height);

    void clearScreen(NXColor color);
    void clearScreen(uint8_t r, uint8_t g, uint8_t b);

    void drawLine(int x1, int y1, int x2, int y2, NXColor color);

    void drawRect(int x1, int y1, int x2, int y2, NXColor color);
    void drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
    void drawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
    void drawRect(NXRect *rect, NXColor color);

    void fillRect(int x1, int y1, int x2, int y2, NXColor color);
    void fillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
    void fillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b);
    void fillRect(NXRect *rect, NXColor color);

    void drawPixel(int x, int y, NXColor color);
    void drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

    void setClip(int x, int y, int w, int h);
    void setClip(NXRect *rect);
    void clearClip();
    bool isClipSet();
    void clip(SDL_Rect &srcrect, SDL_Rect &dstrect);
    void clipScaled(SDL_Rect &srcrect, SDL_Rect &dstrect);

    void saveScreenshot();

    void drawSpotLight(int x, int y, Object* o, int r = 255, int g = 255, int b = 255, int upscale = 6);

    void tintScreen();
    void flip();
    SDL_Renderer* renderer();
    SDL_Window* window();
    Font font;
    Tileset tileset;
    Sprites sprites;

  private:
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    int _current_res = -1;
    bool _need_clip = false;
    SDL_Rect _clip_rect;
    SDL_Texture* _spot_light;
    bool _fullscreen = false;

  protected:
    friend class Singleton<Renderer>;

    Renderer();
    ~Renderer();
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
};

void inline Renderer::drawSurface(Surface *src, int dstx, int dsty)
{
  drawSurface(src, dstx, dsty, 0, 0, src->width(), src->height());
}

void inline Renderer::drawRect(int x1, int y1, int x2, int y2, NXColor color)
{
  drawRect(x1, y1, x2, y2, color.r, color.g, color.b);
}

void inline Renderer::drawRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{
  drawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b);
}

void inline Renderer::drawRect(NXRect *rect, NXColor color)
{
  drawRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b);
}

void inline Renderer::fillRect(int x1, int y1, int x2, int y2, NXColor color)
{
  fillRect(x1, y1, x2, y2, color.r, color.g, color.b);
}

void inline Renderer::fillRect(NXRect *rect, uint8_t r, uint8_t g, uint8_t b)
{
  fillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), r, g, b);
}

void inline Renderer::fillRect(NXRect *rect, NXColor color)
{
  fillRect(rect->x, rect->y, rect->x + (rect->w - 1), rect->y + (rect->h - 1), color.r, color.g, color.b);
}

void inline Renderer::drawPixel(int x, int y, NXColor color)
{
  drawPixel(x, y, color.r, color.g, color.b);
}

void inline Renderer::clearScreen(NXColor color)
{
  clearScreen(color.r, color.g, color.b);
}

void inline Renderer::setClip(NXRect *rect)
{
  setClip(rect->x, rect->y, rect->w, rect->h);
}



}; // namespace Graphics
}; // namespace NXE
#endif
