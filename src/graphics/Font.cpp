#include "Font.h"

#include "../ResourceManager.h"
#include "../common/json.hpp"
#include "../common/misc.h"
#include "../common/stat.h"
#include "Renderer.h"
#include "pngfuncs.h"
#include "../autogen/sprites.h"
#include "../common/utf8.h"
#include "../config.h"
#include "../game.h"
#include "../nx.h"

#include <fstream>
#include <iostream>

using namespace NXE::Graphics;

namespace NXE
{
namespace Graphics
{

Font::Font()
    : _height(0)
    , _base(0)
{
}

bool Font::load(const std::string &font)
{
  cleanup();
  stat("Loading font file %s", font.c_str());

  // special empty glyph
  _glyphs[0] = Font::Glyph{0, 0, 0, 0, 0, 0, 0, 0, 0};

  std::string path = ResourceManager::getInstance()->getLocalizedPath(font);
  stat("Loading font file %s", path.c_str());
  std::ifstream fl;
  fl.open(widen(path), std::ifstream::in | std::ifstream::binary);
  if (fl.is_open())
  {
    nlohmann::json fontdef = nlohmann::json::parse(fl);

    _height = fontdef["common"]["lineHeight"].get<uint32_t>();
    _base   = fontdef["common"]["base"].get<uint32_t>();

    for (auto glyph : fontdef["chars"])
    {
      _glyphs[glyph["id"].get<uint32_t>()] = Font::Glyph{
          glyph["id"].get<uint32_t>(),       glyph["page"].get<uint32_t>(),    glyph["x"].get<uint32_t>(),
          glyph["y"].get<uint32_t>(),        glyph["width"].get<uint32_t>(),   glyph["height"].get<uint32_t>(),
          glyph["xadvance"].get<uint32_t>(), glyph["xoffset"].get<uint32_t>(), glyph["yoffset"].get<uint32_t>()};
    }

    for (auto atlas : fontdef["pages"])
    {
      std::string atlaspath = ResourceManager::getInstance()->getLocalizedPath(atlas.get<std::string>());
      SDL_Surface *surf     = png_load_surface(atlaspath);
      _atlases.push_back(SDL_CreateTextureFromSurface(Renderer::getInstance()->renderer(), surf));
      SDL_FreeSurface(surf);
    }
  }
  else
  {
    staterr("Error opening font file %s", path.c_str());
    return false;
  }

  return true;
}

void Font::cleanup()
{
  _height = 0;
  _base   = 0;
  _glyphs.clear();
  for (auto atlas : _atlases)
  {
    SDL_DestroyTexture(atlas);
  }
  _atlases.clear();
}

Font::~Font()
{
  cleanup();
}

uint32_t Font::height() const
{
  return _height;
}

uint32_t Font::base() const
{
  return _base;
}

const Font::Glyph &Font::glyph(uint32_t codepoint)
{
  if (_glyphs.find(codepoint) != _glyphs.end())
  {
    return _glyphs.at(codepoint);
  }
  else
  {
    staterr("No glyph for codepoint %d", codepoint);
    return _glyphs.at(0);
  }
}

SDL_Texture *Font::atlas(uint32_t idx)
{
  return _atlases.at(idx);
}

uint32_t Font::draw(int x, int y, const std::string &text, uint32_t color, bool isShaded)
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
    char32_t ch = utf8::next(it, text.end());
    Glyph glyph = this->glyph(ch);
    SDL_Texture *atlas  = this->atlas(glyph.atlasid);

    if (ch == '=' && game.mode != GM_CREDITS)
    {
      if (_rendering)
      {
        int offset = (int)round(((double)height() / (double)Renderer::getInstance()->scale - 6.) / 2.);
        Renderer::getInstance()->sprites.drawSprite((x / Renderer::getInstance()->scale), (y / Renderer::getInstance()->scale) + offset, SPR_TEXTBULLET);
      }
    }
    else if (_rendering && ch != ' ')
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
      if (isShaded)
      {
        shdrect.x = x + glyph.xoffset;
        shdrect.y = y + glyph.yoffset + _shadowOffset * Renderer::getInstance()->scale;
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

    if (ch == ' ')
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

uint32_t Font::getWidth(const std::string &text)
{
  _rendering = false;

  uint32_t wd = draw(0, 0, text);

  _rendering = true;

  return wd;
}

uint32_t Font::getHeight() const
{
  return height() / Renderer::getInstance()->scale;
}

uint32_t Font::getBase() const
{
  return base() / Renderer::getInstance()->scale;
}

}; // namespace Graphics
}; // namespace NXE
