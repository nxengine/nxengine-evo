
#include "slope.h"

#include "common/stat.h"
#include "game.h"
#include "graphics/Renderer.h"
#include "map.h"
#include "nx.h"

using namespace NXE::Graphics;

//#define DEBUG_SLOPE
static SlopeTable slopetable[SLOPE_LAST + 1];

// creates the slope tables
bool initslopetable(void)
{
  int x, y, ya, mx;
  int curtable, opposing_table;
  int inverttable, invertfliptable;
  int flipmx, flipy;

  stat("initslopetable: generating slopetables.");
  memset(slopetable, 0, sizeof(slopetable));

  ya = TILE_H - 1;
  for (x = 0; x < TILE_W * 2; x++)
  {
    if (x < TILE_W)
    {
      mx              = x;
      curtable        = SLOPE_FWD1;
      opposing_table  = SLOPE_BACK2;
      inverttable     = SLOPE_CEIL_BACK1;
      invertfliptable = SLOPE_CEIL_FWD2;
    }
    else
    {
      mx              = x - TILE_W;
      curtable        = SLOPE_FWD2;
      opposing_table  = SLOPE_BACK1;
      inverttable     = SLOPE_CEIL_BACK2;
      invertfliptable = SLOPE_CEIL_FWD1;
    }

    for (y = ya; y < TILE_H; y++)
    {
      flipmx = TILE_W - 1 - mx;
      flipy  = TILE_H - 1 - y;

      slopetable[curtable].table[mx][y]                = 1;
      slopetable[opposing_table].table[flipmx][y]      = 1;
      slopetable[inverttable].table[mx][flipy]         = 1;
      slopetable[invertfliptable].table[flipmx][flipy] = 1;
    }

    if (x & 1)
      ya--;
  }

  return 0;
}

/*
void c------------------------------() {}
*/

// X and Y are non-CSFd pixel coordinates relative to the upper-left of the map.
// if the given pixel is inside of a slope, returns the slope type 1-8. else, returns 0.
uint8_t ReadSlopeTable(int x, int y)
{
  int mx, my;
  int slopetype;
  uint8_t t;

#ifdef DEBUG_SLOPE
  DrawSlopeTablesOnTiles();
#endif

  // convert coordinates into a tile and check if the tile is a slope tile
  mx = (x / TILE_W);
  my = (y / TILE_H);

  if (mx < 0 || my < 0 || mx >= map.xsize || my >= map.ysize)
    return 0;

  t = map.tiles[mx][my];

  if (tileattr[t] & TA_SLOPE)
  {
    slopetype = (tilecode[t] & 0x07) + 1; // extract slope type from tile code

    // get offset from the tile
    x %= TILE_W;
    y %= TILE_H;

    if (slopetable[slopetype].table[x][y])
      return slopetype;
  }

  return 0;
}

// returns true if any of the points in the given point list
// are on the solid portion of a slope tile.
bool IsSlopeAtPointList(Object *o, SIFPointList *points)
{
  int x, y, i;

  for (i = 0; i < points->count; i++)
  {
    x = (o->x / CSFI) + points->point[i].x;
    y = (o->y / CSFI) + points->point[i].y;
    if (ReadSlopeTable(x, y))
      return 1;
  }

  return 0;
}

/*
void c------------------------------() {}
*/

// returns nonzero (the slope type) if the object is standing on a slope.
int CheckStandOnSlope(Object *o)
{
  int x, y, st;

  y = (o->y / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y2 + 1;
  x = (o->x / CSFI);

  if ((st = ReadSlopeTable(x + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x1, y)))
    return st;
  if ((st = ReadSlopeTable(x + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x2, y)))
    return st;

  return 0;
}

// returns nonzero (the slope type) if the objects blocku should be set
// because of a ceiling slope.
int CheckBoppedHeadOnSlope(Object *o)
{
  int x, y, st;

  y = (o->y / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y1 - 1;
  x = (o->x / CSFI);

  // without this, you get stuck in the save area below Gum Door in Grasstown
  // if (o == player) y += 4;

  if ((st = ReadSlopeTable(x + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x1, y)))
    return st;
  if ((st = ReadSlopeTable(x + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x2, y)))
    return st;

  return 0;
}

// move an object laterally, and have it climb slopes as it approaches them.
// We also have to move the object down as it goes down the slope.
// Otherwise, it would "skip" down the slope ungracefully.
// returns 1 if the object was blocked by a wall.
bool movehandleslope(Object *o, int xinertia)
{
  int xoff, opposing_x;
  int newx, newy, oldy;
  char blocked_wall;

  if (!xinertia)
    return 0;

  // for objects which don't follow slope, just treat the slope as a blockl/r
  if (!(o->nxflags & NXFLAG_FOLLOW_SLOPE))
  {
    if (xinertia > 0)
    {
      if (o->blockr)
        return 1;
    }
    else
    {
      if (o->blockl)
        return 1;
    }

    o->x += xinertia;
    return 0;
  }

  newx = o->x;
  newy = o->y;

  // determine which side of the bounding box to use based on which way
  // we're traveling
  if (xinertia > 0)
  { // moving right (right side of slopebox hits slopes first)
    opposing_x = Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x1;
    xoff       = Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x2;
  }
  else
  { // move left (left side of slopebox hits slopes first)
    opposing_x = Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x2;
    xoff       = Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.x1;
  }

  // check the opposing side at y+1 to see if we were standing on a slope before the move.
  uint8_t old_floor_slope, old_ceil_slope;
  old_floor_slope = ReadSlopeTable((newx / CSFI) + opposing_x, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y2 + 1);

  old_ceil_slope = ReadSlopeTable((newx / CSFI) + opposing_x, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y1 - 1);

  // move the object
  newx += xinertia;

  // check the opposing side again and if now we're not standing any more,
  // we moved down the slope, so add +1 to the object's Y coordinate.
  if (old_floor_slope
      && !ReadSlopeTable((newx / CSFI) + opposing_x, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y2 + 1))
  {
    bool walking_down = false;

    // only trigger if it's the correct slope type so that we would be walking down it if
    // we were going in the direction we're going. prevents being shoved down 1px when
    // exiting the top of a slope.
    if (xinertia < 0)
    {
      if (old_floor_slope == SLOPE_FWD1 || old_floor_slope == SLOPE_FWD2)
      {
        walking_down = true;
      }
    }
    else if (xinertia > 0)
    {
      if (old_floor_slope == SLOPE_BACK1 || old_floor_slope == SLOPE_BACK2)
      {
        walking_down = true;
      }
    }

    if (walking_down)
    {
      newy += (1 * CSFI);
    }
  }

  // the same for ceiling slopes
  if (old_ceil_slope && !ReadSlopeTable((newx / CSFI) + opposing_x, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y1 - 1))
  {
    bool moveme = false;

    if (xinertia < 0)
    {
      if (old_ceil_slope == SLOPE_CEIL_BACK1 || old_ceil_slope == SLOPE_CEIL_BACK2)
      {
        moveme = true;
      }
    }
    else if (xinertia > 0)
    {
      if (old_ceil_slope == SLOPE_CEIL_FWD1 || old_ceil_slope == SLOPE_CEIL_FWD2)
      {
        moveme = true;
      }
    }

    if (moveme)
    { // moving down (actually up) the "descending" (closer to real ceil) portion
      // of a ceiling slope tile. Reverse of floor slope thingy above.
      newy -= (1 * CSFI);
    }
  }

  // check the coordinate and see if it's inside a slope tile.
  // if so, move the object up 1 Y pixel.
  uint8_t moved_into_ceil_slope = ReadSlopeTable((newx / CSFI) + xoff, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y1);
  if (moved_into_ceil_slope)
  {
    newy += (1 * CSFI);
  }

  uint8_t moved_into_floor_slope = ReadSlopeTable((newx / CSFI) + xoff, (newy / CSFI) + Renderer::getInstance()->sprites.sprites[o->sprite].slopebox.y2);
  if (moved_into_floor_slope)
  {
    newy -= (1 * CSFI);
  }

  // can't move if blocked by a wall. but if we've moved up or down 1px, be sure and update
  // the blockr/l state before declaring we can't move--otherwise we can get stuck at the
  // top of a slope with the bottom blockr/l stuck at the top px of the adjacent solid tile.
  oldy = o->y;
  o->y = newy;

  if (xinertia > 0)
  {
    if (oldy != newy)
      o->UpdateBlockStates(RIGHTMASK);

    blocked_wall = o->blockr;
  }
  else
  {
    if (oldy != newy)
      o->UpdateBlockStates(LEFTMASK);

    blocked_wall = o->blockl;
  }

  if (blocked_wall)
  { // we can't actually move...so reset Y position
    o->y = oldy;
  }
  else
  { // can move...complete the move by setting the X position too
    o->x = newx;
  }

  return blocked_wall;
}

/*
void c------------------------------() {}
*/

#ifdef DEBUG_SLOPE
// debug crap

void DrawSlopeTablesOnTiles()
{
  static int lastmap = -1;

  if (game.curmap != lastmap)
  {
    lastmap = game.curmap;
    for (int i = 0; i < 256; i++)
    {
      if (tileattr[i] & TA_SLOPE)
      {
        DrawSlopeTableOnTile((tilecode[i] & 7) + 1, i);
      }
    }
  }
}

void DrawSlopeTableOnTile(int table, int tile)
{
  SDL_Rect dstrect;
  int x, y;
  extern SDL_Surface *tileset;

  for (y = 0; y < TILE_H; y++)
  {
    for (x = 0; x < TILE_W; x++)
    {
      dstrect.x = (tile & 15) << 5;
      dstrect.y = (tile >> 4) << 5;
      dstrect.w = 2;
      dstrect.h = 2;

      dstrect.x += x * 2;
      dstrect.y += y * 2;

      if (slopetable[table].table[x][y])
      {
        if (table > 4) // floor slopes
          SDL_FillRect(tileset, &dstrect, SDL_MapRGB(tileset->format, 0, 255, 0));
        else
          SDL_FillRect(tileset, &dstrect, SDL_MapRGB(tileset->format, 255, 0, 0));
      }
    }
  }
}

void dumpslopetable(int t)
{
  int x, y;
  char buffer[80];

  stat("\nDumping slope table %d:", t);
  for (y = 0; y < TILE_H; y++)
  {
    for (x = 0; x < TILE_W; x++)
    {
      buffer[x] = slopetable[t].table[x][y] + '0';
    }
    buffer[x] = 0;
    stat("%s", buffer);
  }
}
#endif
