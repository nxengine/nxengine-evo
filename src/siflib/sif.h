
#ifndef _SIF_H
#define _SIF_H

#include "../common/misc.h" // stprintf

// a sprite has certain settings global to the whole sprite.
// each sprite also contains a certain number of frames.
// each frame contains one or more directions, for example for when that
// frame is facing left or right, and sometimes up or down.
// the number of directions is settable per-sprite, but each frame in
// a given sprite always has the same number of directions.

// this layout is somewhat wasteful of memory what with all static allocations and few pointers
// or dynamically-sized substructures. the reason I did it this way is for speed; hoping the
// tradeoff in memory will mean less indirections to access the sprite data, which is generally
// read several times per frame per sprite.

#define SIF_MAX_DIRS 4
#define SIF_MAX_BLOCK_POINTS 4

struct SIFPoint
{
  int16_t x, y;

  void set(int x, int y)
  {
    this->x = x;
    this->y = y;
  }

  void offset(int dx, int dy)
  {
    x += dx;
    y += dy;
  }

  bool equ(int x, int y)
  {
    return (this->x == x && this->y == y);
  }

  const char *ToString()
  {
    return stprintf("[%d, %d]", this->x, this->y);
  }

  bool operator==(const SIFPoint &other) const
  {
    return (this->x == other.x && this->y == other.y);
  }

  bool operator!=(const SIFPoint &other) const
  {
    return (this->x != other.x || this->y != other.y);
  }
};

typedef SIFPoint Point;

struct SIFRect
{
  int16_t x1, y1, x2, y2;

  void set(int x1, int y1, int x2, int y2)
  {
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
  }

  void offset(int dx, int dy)
  {
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;
  }

  bool equ(int x1, int y1, int x2, int y2)
  {
    return (this->x1 == x1 && this->y1 == y1 && this->x2 == x2 && this->y2 == y2);
  }

  const char *ToString()
  {
    return stprintf("[%d, %d] - [%d, %d]", this->x1, this->y1, this->x2, this->y2);
  }

  bool operator==(const SIFRect &other) const
  {
    return (x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2);
  }

  bool operator!=(const SIFRect &other) const
  {
    return (x1 != other.x1 || y1 != other.y1 || x2 != other.x2 || y2 != other.y2);
  }
};

struct SIFPointList
{
  SIFPoint point[SIF_MAX_BLOCK_POINTS];
  int count;

  SIFPoint &operator[](const int nIndex)
  {
    return point[nIndex];
  }

  bool operator==(const SIFPointList &other) const
  {
    if (other.count != count)
      return false;

    for (int i = 0; i < count; i++)
      if (point[i] != other.point[i])
        return false;

    return true;
  }

  bool operator!=(const SIFPointList &other) const
  {
    return !(*this == other);
  }

  void offset(int dx, int dy)
  {
    for (int i = 0; i < count; i++)
    {
      point[i].x += dx;
      point[i].y += dy;
    }
  }
};

struct SIFDir
{
  // the offset on the sprite sheet where this frame is located.
  SIFPoint sheet_offset;

  // when a sprite is drawn, it's sprite will be offset such that this pixel is
  // placed at the (x,y) coordinates that were given for the sprite to be drawn at.
  // i.e., it's a "hot point".
  // this also offsets the bounding boxes and blockl/r/u/d points but not the
  // action points or spawn point.
  // it is not super-well tested and isn't intended to be frequently used on objects which
  // interact with the world or are solid, etc. mostly it is for use with objects
  // such as the caret effects etc, so that their o->x & o->y can represent their center
  // point instead of their upper-left corner.
  SIFPoint drawpoint;

  //	a pixel of "noteworthiness" on the sprite.
  //  * for Player: position he holds his gun at
  //  * for Guns: position to spawn the shots at
  //	* for everything else: whatever they want it for
  SIFPoint actionpoint;

  // optional secondary action point
  SIFPoint actionpoint2;

  // optional additional/extra per-frame/dir bbox
  // (has no effect to engine, it's just metadata. but object AI can copy the
  //  per-frame bbox over the real bbox when changing the frame).
  SIFRect pf_bbox;
};

struct SIFFrame
{
  SIFDir dir[SIF_MAX_DIRS];
};

// a design goal here is that this structure MUST STAY POD so
// no virtuals, constructors, classes for members or anything else
// dodgy that might endanger that. we should able to zero out a SIFSprite
// with memset and worry about nothing. yes it has a few member functions
// but those are just a syntactical detail and should be harmless.
struct SIFSprite
{
  void Init();
  void Zero();
  void FreeData();

  void AddFrame(SIFFrame *newframe);
  void InsertFrame(SIFFrame *newframe, int insertbefore);
  void DeleteFrame(int index);
  void SetNumFrames(int newcount);
  SIFDir *dir(int frame, int dir);

  void CopyFrom(SIFSprite *other);
  SIFSprite *Duplicate();

  // ---------------------------------------

  int w, h;
  uint8_t spritesheet; // # of spritesheet sprite is located on

  int nframes;
  int ndirs;
  SIFFrame *frame;

  SIFRect bbox[SIF_MAX_DIRS];     // bounding box for hit detection with other sprites
  SIFRect solidbox; // bounding box for solidity w/ other objects
  SIFRect slopebox; // bounding box for moving up and down slopes (generated at runtime)

  // when an object is spawned or changed from one type to another, this point is subtracted
  // from it's x,y coordinates, so that this point ends up being centered on the given x,y coords.
  SIFPoint spawn_point;

  // points to check for setting the corresponding block vars on the object
  // (which determine whether it's blocked on that side by a solid wall or object).
  SIFPointList block_l;
  SIFPointList block_r;
  SIFPointList block_u;
  SIFPointList block_d;
};

#endif
