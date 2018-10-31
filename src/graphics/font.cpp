
#include "font.h"

#include "../autogen/sprites.h"
#include "../common/stat.h"
#include "../common/utf8.h"
#include "../config.h"
#include "../game.h"
#include "../nx.h"
#include "bmfont.h"
#include "graphics.h"
#include "sprites.h"

#include <SDL.h>

#define SHADOW_OFFSET 1 // distance of drop shadows

static bool initilized    = false;
static bool rendering     = true;
static bool shrink_spaces = true;

static BMFont whitefnt;

extern SDL_Renderer *renderer;

bool font_init(void)
{
  whitefnt.load(std::string("font_" + std::to_string(SCALE) + ".fnt"));

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
  x *= SCALE;
  y *= SCALE;

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
        Sprites::draw_sprite((x / SCALE), (y / SCALE) + 1, SPR_TEXTBULLET);
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

      if (Graphics::is_set_clip())
        Graphics::clip(srcrect, dstrect);
      // TODO: I'm not sure, but it looks like original text is outlined, not shadowed
      if (is_shaded)
      {
        shdrect.x = x + glyph.xoffset + SHADOW_OFFSET;
        shdrect.y = y + glyph.yoffset + SHADOW_OFFSET;
        shdrect.w = glyph.w;
        shdrect.h = glyph.h;
        SDL_SetTextureColorMod(atlas, 0, 0, 0);
        SDL_RenderCopy(renderer, atlas, &srcrect, &shdrect);
        SDL_SetTextureColorMod(atlas, 255, 255, 255);
      }
      SDL_SetTextureColorMod(atlas, r, g, b);
      SDL_RenderCopy(renderer, atlas, &srcrect, &dstrect);
      SDL_SetTextureColorMod(atlas, 255, 255, 255);
    }

    if (ch == ' ' && shrink_spaces)
    { // 10.5 px for spaces - make smaller than they really are - the default
      x += (SCALE == 1) ? 5 : 10;
      if (i & 1)
        x++;
    }
    else if (ch == '=' && game.mode != GM_CREDITS)
    {
      x += 7 * SCALE;
    }
    else
    {
      x += glyph.xadvance;
    }
    i++;
  }

  // return the final width of the text drawn
  return (x - orgx) / SCALE;
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
  return whitefnt.height() / SCALE;
}

int GetFontBase()
{
  return whitefnt.base() / SCALE;
}
