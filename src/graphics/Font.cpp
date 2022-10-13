#include "Font.h"

#include "../ResourceManager.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "Renderer.h"
#include "../autogen/sprites.h"
#include "../config.h"
#include "../game.h"
#include "../nx.h"

#include <SDL_image.h>
#include <json.hpp>
#include <utf8.h>
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

bool Font::load()
{
  cleanup();
  std::string font = std::string("font_1.fnt");
  LOG_DEBUG("Loading font file {}", font.c_str());

  // special empty glyph
  _glyphs[0] = Font::Glyph{0, 0, 0, 0, 0, 0, 0, 0, 0};

  std::string path = ResourceManager::getInstance()->getPath(font);
  ResourceManager::getInstance()->fileExists(path);

  LOG_DEBUG("Loading font file {}", path.c_str());

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
      std::string atlaspath = ResourceManager::getInstance()->getPath(atlas.get<std::string>());
      SDL_Surface *surf     = IMG_Load(atlaspath.c_str());
      _atlases.push_back(SDL_CreateTextureFromSurface(Renderer::getInstance()->renderer(), surf));
      SDL_FreeSurface(surf);
    }
  }
  else
  {
    LOG_ERROR("Error opening font file {}", path.c_str());
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
  _upscale = 1;
}

Font::~Font()
{
  cleanup();
}

const Font::Glyph &Font::glyph(uint32_t codepoint)
{
  if (_glyphs.find(codepoint) != _glyphs.end())
  {
    return _glyphs.at(codepoint);
  }
  else
  {
    LOG_WARN("No glyph for codepoint {}", codepoint);
    return _glyphs.at(0);
  }
}

SDL_Texture *Font::atlas(uint32_t idx)
{
  return _atlases.at(idx);
}

uint32_t Font::draw(int x, int y, const std::string &text, uint32_t color, bool isShaded)
{
  int orgx = x;
  int i    = 0;
  SDL_Rect dstrect;
  SDL_Rect shdrect;
  SDL_Rect srcrect;

  int r, g, b;

  r = ((color >> 16) & 0xFF);
  g = ((color >> 8) & 0xFF);
  b = ((color)&0xFF);

  std::string::const_iterator it = (rtl() ? text.end() : text.begin());
  while (it != (rtl() ? text.begin() : text.end()) )
  {
    char32_t ch;
    if (rtl()) ch = utf8::prior(it, text.begin());
    else ch = utf8::next(it, text.end());

    Glyph glyph = this->glyph(ch);
    SDL_Texture *atlas  = this->atlas(glyph.atlasid);

    if (ch == '=' && game.mode != GM_CREDITS)
    {
      if (_rendering)
      {
        int offset = (int)round(((double)_height - 6.) / 2.);
        Renderer::getInstance()->sprites.drawSprite(x, y + offset, SPR_TEXTBULLET);
      }
    }
    else if (_rendering && ch != ' ')
    {
      dstrect.x = x + (glyph.xoffset * _upscale);
      dstrect.y = y + (glyph.yoffset * _upscale);
      dstrect.w = glyph.w * _upscale;
      dstrect.h = glyph.h * _upscale;

      srcrect.x = glyph.x;
      srcrect.y = glyph.y;
      srcrect.w = glyph.w;
      srcrect.h = glyph.h;

      if (Renderer::getInstance()->isClipSet())
      {
        if (_upscale > 1)
          Renderer::getInstance()->clip(srcrect, dstrect);
        else
          Renderer::getInstance()->clipScaled(srcrect, dstrect);
      }
      if (isShaded)
      {
        shdrect.x = x + (glyph.xoffset * _upscale);
        shdrect.y = y + (glyph.yoffset * _upscale + _shadowOffset);
        shdrect.w = glyph.w * _upscale;
        shdrect.h = glyph.h * _upscale;
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
      if (rtl())
      {
        x -= 5;
        if (i & 1)
          x--;
      }
      else
      {
        x += 5;
        if (i & 1)
          x++;
      }
    }
    else if (ch == '=' && game.mode != GM_CREDITS)
    {
      if (rtl()) x -= 7;
      else x += 7;
    }
    else
    {
      if (rtl()) x -= glyph.xadvance * _upscale;
      else x += glyph.xadvance * _upscale;
    }
    i++;
  }

  // return the final width of the text drawn
  return abs(x - orgx);
}

uint32_t Font::drawLTR(int x, int y, const std::string &text, uint32_t color, bool isShaded)
{
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
  while (it != text.end() )
  {
    char32_t ch;
    ch = utf8::next(it, text.end());

    Glyph glyph = this->glyph(ch);
    SDL_Texture *atlas  = this->atlas(glyph.atlasid);

    if (ch == '=' && game.mode != GM_CREDITS)
    {
      if (_rendering)
      {
        int offset = (int)round(((double)_height - 6.) / 2.);
        Renderer::getInstance()->sprites.drawSprite(x, y + offset, SPR_TEXTBULLET);
      }
    }
    else if (_rendering && ch != ' ')
    {
      dstrect.x = x + (glyph.xoffset * _upscale);
      dstrect.y = y + (glyph.yoffset * _upscale);
      dstrect.w = glyph.w * _upscale;
      dstrect.h = glyph.h * _upscale;

      srcrect.x = glyph.x;
      srcrect.y = glyph.y;
      srcrect.w = glyph.w;
      srcrect.h = glyph.h;

      if (Renderer::getInstance()->isClipSet())
      {
        if (_upscale > 1)
          Renderer::getInstance()->clip(srcrect, dstrect);
        else
          Renderer::getInstance()->clipScaled(srcrect, dstrect);
      }
      if (isShaded)
      {
        shdrect.x = x + (glyph.xoffset * _upscale);
        shdrect.y = y + glyph.yoffset * _upscale + _shadowOffset;
        shdrect.w = glyph.w * _upscale;
        shdrect.h = glyph.h * _upscale;
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
      x += 5;
      if (i & 1)
        x++;
    }
    else if (ch == '=' && game.mode != GM_CREDITS)
    {
      x += 7;
    }
    else
    {
      x += glyph.xadvance * _upscale;
    }
    i++;
  }

  // return the final width of the text drawn
  return abs(x - orgx);
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
  return _height;
}

uint32_t Font::getBase() const
{
  return _base;
}

}; // namespace Graphics
}; // namespace NXE
