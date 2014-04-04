
#ifndef _TILESET_H
#define _TILESET_H
#include "nxsurface.h"

#define TILE_W				16
#define TILE_H				16

namespace Tileset
{
	bool Init();
	void Close();
	
	bool Load(int new_tileset);
	void Reload();
	void draw_tile(int x, int y, int t);
	
	NXSurface *GetSurface();
};


#endif
