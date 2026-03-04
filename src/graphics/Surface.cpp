#include "Surface.h"
#include "Renderer.h"
#include "../Utils/Logger.h"
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
    LOG_ERROR("Surface::LoadImage: load failed of '{}'! {}", pbm_name, SDL_GetError());
    return false;
  }

  _width = image->w;
  _height = image->h;

  if (use_colorkey)
  {
    SDL_SetColorKey(image, SDL_TRUE, SDL_MapRGB(image->format, 0, 0, 0));
  }

  _texture = SDL_CreateTextureFromSurface(Renderer::getInstance()->renderer(), image);

  SDL_FreeSurface(image);

  if (!_texture)
  {
    LOG_ERROR("Surface::LoadImage: SDL_CreateTextureFromSurface failed: {}", SDL_GetError());
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
  return _width;
}

int Surface::height()
{
  return _height;
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
