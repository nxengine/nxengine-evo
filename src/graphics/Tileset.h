#ifndef _TILESET_H
#define _TILESET_H

#include "Surface.h"

#define TILE_W 16
#define TILE_H 16

namespace NXE
{
namespace Graphics
{

class Tileset
{
  public:
    Tileset();
    ~Tileset();
    bool init();
    void close();

    bool load(int new_tileset);
    void reload();

    void drawTile(int x, int y, int t);
  private:
    Surface *_tileset = nullptr;
    int _current_tileset = -1;
};

}; // namespace Graphics
}; // namespace Tileset

#endif
