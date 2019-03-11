
#include "font.h"

#include "../autogen/sprites.h"
#include "../common/stat.h"
#include "../common/utf8.h"
#include "../config.h"
#include "../game.h"
#include "../nx.h"
#include "bmfont.h"
#include "Renderer.h"
#include "sprites.h"

using namespace NXE::Graphics;

#include <SDL.h>

#define SHADOW_OFFSET 1 // distance of drop shadows

static bool initilized    = false;
static bool rendering     = true;
static bool shrink_spaces = true;

static BMFont whitefnt;

bool font_init(void)
{
  whitefnt.load(std::string("font_" + std::to_string(Renderer::getInstance()->scale) + ".fnt"));

  initilized = true;
  return 0;
}

void font_close(void)
{
  if (!initilized)
    return;

  whitefnt.cleanup();
}

bool font_reload()
{
  if (!initilized)
    return 0;

  whitefnt.cleanup();

  return font_init();
}

// draw a text string
int font_draw(int x, int y, const std::string &text, uint32_t color, bool is_shaded)
{
  x *= Renderer::getInstance()->scale;
  y *= Renderer::getInstance()->scale;

  int orgx = x;
  int i    = 0;
  SDL_Rect dstrect;
  SDL_Rect shdrect;
  SDL_Rect srcrect;

  int r, g, b;

  r = ((color >> 16) & 0xFF);
  g = ((color >> 8) & 0xFF);
  b = ((color)&0xFF);

  std::string::const_iterator it = text.begin();
  while (it != text.end())
  {
    char32_t ch         = utf8::next(it, text.end());
    BMFont::Glyph glyph = whitefnt.glyph(ch);
    SDL_Texture *atlas  = whitefnt.atlas(glyph.atlasid);

    if (ch == '=' && game.mode != GM_CREDITS)
    {
      if (rendering)
      {
        int offset = (int)round(((double)whitefnt.height() / (double)Renderer::getInstance()->scale - 6.) / 2.);
        Sprites::drawSprite((x / Renderer::getInstance()->scale), (y / Renderer::getInstance()->scale) + offset, SPR_TEXTBULLET);
      }
    }
    else if (rendering && ch != ' ')
    {
      dstrect.x = x + glyph.xoffset;
      dstrect.y = y + glyph.yoffset;
      dstrect.w = glyph.w;
      dstrect.h = glyph.h;

      srcrect.x = glyph.x;
      srcrect.y = glyph.y;
      srcrect.w = dstrect.w;
      srcrect.h = dstrect.h;

      if (Renderer::getInstance()->isClipSet())
        Renderer::getInstance()->clipScaled(srcrect, dstrect);
      if (is_shaded)
      {
        shdrect.x = x + glyph.xoffset;
        shdrect.y = y + glyph.yoffset + SHADOW_OFFSET * Renderer::getInstance()->scale;
        shdrect.w = glyph.w;
        shdrect.h = glyph.h;
        SDL_SetTextureColorMod(atlas, 0, 0, 0);
        SDL_RenderCopy(Renderer::getInstance()->renderer(), atlas, &srcrect, &shdrect);
        SDL_SetTextureColorMod(atlas, 255, 255, 255);
      }
      SDL_SetTextureColorMod(atlas, r, g, b);
      SDL_RenderCopy(Renderer::getInstance()->renderer(), atlas, &srcrect, &dstrect);
      SDL_SetTextureColorMod(atlas, 255, 255, 255);
    }

    if (ch == ' ' && shrink_spaces)
    { // 10.5 px for spaces - make smaller than they really are - the default
      x += (Renderer::getInstance()->scale == 1) ? 5 : 10;
      if (i & 1)
        x++;
    }
    else if (ch == '=' && game.mode != GM_CREDITS)
    {
      x += 7 * Renderer::getInstance()->scale;
    }
    else
    {
      x += glyph.xadvance;
    }
    i++;
  }

  // return the final width of the text drawn
  return (x - orgx) / Renderer::getInstance()->scale;
}

int GetFontWidth(const std::string &text, bool is_shaded)
{
  int wd;

  rendering     = false;
  shrink_spaces = !is_shaded;

  wd = font_draw(0, 0, text);

  rendering     = true;
  shrink_spaces = true;

  return wd;
}

int GetFontHeight()
{
  return whitefnt.height() / Renderer::getInstance()->scale;
}

int GetFontBase()
{
  return whitefnt.base() / Renderer::getInstance()->scale;
}
