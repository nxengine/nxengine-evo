
#ifndef _TILESET_H
#define _TILESET_H

#define TILE_W 16
#define TILE_H 16

#include "Surface.h"

namespace Tileset
{
bool init();
void close();

bool load(int new_tileset);
void reload();
void drawTile(int x, int y, int t);

NXE::Graphics::Surface *getSurface();
}; // namespace Tileset

#endif
