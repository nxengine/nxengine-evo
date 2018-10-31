
// manages the tileset
#include "tileset.h"

#include "../ResourceManager.h"
#include "../config.h"
#include "../nx.h"
#include "graphics.h"

#include <cassert>
#include <cstdio>
using namespace Graphics;

extern const char *tileset_names[]; // from stagedata.cpp

static NXSurface *tileset;
static int current_tileset = -1;

bool Tileset::Init()
{
  tileset         = NULL;
  current_tileset = -1;
  return 0;
}

void Tileset::Close()
{
  delete tileset;
}

/*
void c------------------------------() {}
*/

// load the given tileset into memory, replacing any other tileset.
bool Tileset::Load(int new_tileset)
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
    tileset = NXSurface::FromFile(ResourceManager::getInstance()->getLocalizedPath(fname), true, true);
    if (!tileset)
    {
      return 1;
    }

    current_tileset = new_tileset;
  }

  return 0;
}

// draw the given tile from the current tileset to the screen
void Tileset::draw_tile(int x, int y, int t)
{
  // 16 tiles per row on all tilesheet
  int srcx = (t % 16) * TILE_W;
  int srcy = (t / 16) * TILE_H;

  DrawSurface(tileset, x, y, srcx, srcy, TILE_W, TILE_H);
}

#if defined(CONFIG_FAST_TILEGRID)

void Tileset::draw_tilegrid_begin(size_t max_count)
{
  DrawBatchBegin(max_count);
}

void Tileset::draw_tilegrid_add(int x, int y, int t)
{
  // 16 tiles per row on all tilesheet
  int srcx = (t % 16) * TILE_W;
  int srcy = (t / 16) * TILE_H;

  DrawBatchAdd(tileset, x, y, srcx, srcy, TILE_W, TILE_H);
}

void Tileset::draw_tilegrid_end()
{
  DrawBatchEnd();
}

#else
void Tileset::draw_tilegrid_begin(size_t) {}
void Tileset::draw_tilegrid_add(int x, int y, int t)
{
  return draw_tile(x, y, t);
}
void Tileset::draw_tilegrid_end() {}
#endif

void Tileset::Reload()
{
  if (current_tileset != -1)
  {
    int tileset     = current_tileset;
    current_tileset = -1;
    Load(tileset);
  }
}

/*
void c------------------------------() {}
*/

NXSurface *Tileset::GetSurface()
{
  return tileset;
}
