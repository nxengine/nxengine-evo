
// manages the tileset
#include "tileset.h"

#include "../ResourceManager.h"
#include "../config.h"
#include "../nx.h"
#include "Surface.h"
#include "Renderer.h"

#include <cassert>
#include <cstdio>
using namespace NXE::Graphics;

extern const char *tileset_names[]; // from stagedata.cpp

static Surface *tileset;
static int current_tileset = -1;

bool Tileset::init()
{
  tileset         = NULL;
  current_tileset = -1;
  return 0;
}

void Tileset::close()
{
  delete tileset;
}

/*
void c------------------------------() {}
*/

// load the given tileset into memory, replacing any other tileset.
bool Tileset::load(int new_tileset)
{
  char fname[MAXPATHLEN];

  if (new_tileset != current_tileset)
  {
    if (tileset)
    {
      delete tileset;
      current_tileset = -1;
    }

    sprintf(fname, "Stage/Prt%s.pbm", tileset_names[new_tileset]);

    // always use SDL_DisplayFormat on tilesets; they need to come out of 8-bit
    // so that we can replace the destroyable star tiles without them palletizing.
    tileset = Surface::fromFile(ResourceManager::getInstance()->getLocalizedPath(fname), true);
    if (!tileset)
    {
      return 1;
    }

    current_tileset = new_tileset;
  }

  return 0;
}

// draw the given tile from the current tileset to the screen
void Tileset::drawTile(int x, int y, int t)
{
  // 16 tiles per row on all tilesheet
  int srcx = (t % 16) * TILE_W;
  int srcy = (t / 16) * TILE_H;

  Renderer::getInstance()->drawSurface(tileset, x, y, srcx, srcy, TILE_W, TILE_H);
}

void Tileset::reload()
{
  if (current_tileset != -1)
  {
    int tileset     = current_tileset;
    current_tileset = -1;
    load(tileset);
  }
}

/*
void c------------------------------() {}
*/

Surface *Tileset::getSurface()
{
  return tileset;
}
