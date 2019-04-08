#include "Surface.h"
#include "Renderer.h"
#include "../common/stat.h"
#include "zoom.h"

namespace NXE
{
namespace Graphics
{

Surface::Surface()
    : _texture(nullptr)
    , _width(0)
    , _height(0)
{
}

Surface::~Surface()
{
  cleanup();
}

// load the surface from a .pbm or bitmap file
bool Surface::loadImage(const std::string &pbm_name, bool use_colorkey)
{
  cleanup();

  SDL_Surface *image = SDL_LoadBMP(pbm_name.c_str());
  if (!image)
  {
    staterr("Surface::LoadImage: load failed of '%s'! %s", pbm_name.c_str(), SDL_GetError());
    return false;
  }

  _width = image->w * Renderer::getInstance()->scale;
  _height = image->h * Renderer::getInstance()->scale;

  SDL_Surface *image_scaled = SDL_ZoomSurface(image, Renderer::getInstance()->scale);
  SDL_FreeSurface(image);

  if (use_colorkey)
  {
    SDL_SetColorKey(image_scaled, SDL_TRUE, SDL_MapRGB(image_scaled->format, 0, 0, 0));
  }

  _texture = SDL_CreateTextureFromSurface(Renderer::getInstance()->renderer(), image_scaled);

  SDL_FreeSurface(image_scaled);

  if (!_texture)
  {
    staterr("Surface::LoadImage: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
    return false;
  }

  return true;
}

Surface *Surface::fromFile(const std::string &pbm_name, bool use_colorkey)
{
  Surface *sfc = new Surface;
  if (!sfc->loadImage(pbm_name, use_colorkey))
  {
    delete sfc;
    return nullptr;
  }

  return sfc;
}

int Surface::width()
{
  return _width / Renderer::getInstance()->scale;
}

int Surface::height()
{
  return _height / Renderer::getInstance()->scale;
}

SDL_Texture* Surface::texture()
{
  return _texture;
}

void Surface::cleanup()
{
  if (_texture)
  {
    SDL_DestroyTexture(_texture);
    _texture = nullptr;
  }
}

}; // namespace Graphics
}; // namespace NXE
