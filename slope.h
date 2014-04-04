
#ifndef _SLOPE_H
#define _SLOPE_H
#include "graphics/tileset.h"
#include "object.h"
#include "siflib/sif.h"

// slope types
#define SLOPE_CEIL_FWD1		1
#define SLOPE_CEIL_FWD2		2
#define SLOPE_CEIL_BACK1	3
#define SLOPE_CEIL_BACK2	4

#define SLOPE_BACK1			5
#define SLOPE_BACK2			6
#define SLOPE_FWD1			7
#define SLOPE_FWD2			8

#define SLOPE_LAST			8

struct SlopeTable
{
	uint8_t table[TILE_W][TILE_H];
};
bool initslopetable(void);
uint8_t ReadSlopeTable(int x, int y);
bool IsSlopeAtPointList(Object *o, SIFPointList *points);
int CheckStandOnSlope(Object *o);
int CheckBoppedHeadOnSlope(Object *o);
bool movehandleslope(Object *o, int xinertia);
void DrawSlopeTablesOnTiles();
void DrawSlopeTableOnTile(int table, int tile);
void dumpslopetable(int t);

#endif
