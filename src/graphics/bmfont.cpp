#include "bmfont.h"

#include "../ResourceManager.h"
#include "../common/json.hpp"
#include "../common/misc.h"
#include "../common/stat.h"
#include "Renderer.h"
#include "pngfuncs.h"

#include <fstream>
#include <iostream>

using namespace NXE::Graphics;


BMFont::BMFont()
    : _height(0)
    , _base(0)
{
}

bool BMFont::load(const std::string &font)
{
  cleanup();
  stat("Loading font file %s", font.c_str());

  // special empty glyph
  _glyphs[0] = BMFont::Glyph{0, 0, 0, 0, 0, 0, 0, 0, 0};

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
      _glyphs[glyph["id"].get<uint32_t>()] = BMFont::Glyph{
          glyph["id"].get<uint32_t>(),       glyph["page"].get<uint32_t>(),    glyph["x"].get<uint32_t>(),
          glyph["y"].get<uint32_t>(),        glyph["width"].get<uint32_t>(),   glyph["height"].get<uint32_t>(),
          glyph["xadvance"].get<uint32_t>(), glyph["xoffset"].get<uint32_t>(), glyph["yoffset"].get<uint32_t>()};
    }

    for (auto atlas : fontdef["pages"])
    {
      std::string atlaspath = ResourceManager::getInstance()->getLocalizedPath(atlas.get<std::string>());
      SDL_Surface *surf     = png_load_surface(atlaspath.c_str());
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

void BMFont::cleanup()
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

BMFont::~BMFont()
{
  cleanup();
}

uint32_t BMFont::height() const
{
  return _height;
}

uint32_t BMFont::base() const
{
  return _base;
}

const BMFont::Glyph &BMFont::glyph(uint32_t codepoint)
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

SDL_Texture *BMFont::atlas(uint32_t idx)
{
  return _atlases.at(idx);
}
