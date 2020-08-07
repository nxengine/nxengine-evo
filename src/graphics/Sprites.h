
#ifndef _SPRITES_H
#define _SPRITES_H

#define MAX_SPRITESHEETS 128
#define MAX_SPRITES 512

#include "../siflib/sif.h"
#include "Surface.h"

#include <vector>
#include <string>
#include <cstdint>

namespace NXE
{
namespace Graphics
{

class Sprites
{
public:
  Sprites();
  ~Sprites();

  bool init();
  void close();
  void flushSheets();

  void blitSprite(int x, int y, int s, int frame, uint8_t dir, int xoff, int yoff, int wd, int ht, int alpha = 255);
  void blitSpriteMirrored(int x, int y, int s, int frame, uint8_t dir, int xoff, int yoff, int wd, int ht, int alpha = 255);

  void drawSprite(int x, int y, int s, int frame = 0, uint8_t dir = 0);
  void drawSpriteMirrored(int x, int y, int s, int frame = 0, uint8_t dir = 0);
  void drawSpriteAtDp(int x, int y, int s, int frame = 0, uint8_t dir = 0);
  void drawSpriteClipped(int x, int y, int s, int frame, uint8_t dir, int clipx1, int clipx2, int clipy1, int clipy2);
  void drawSpriteClipWidth(int x, int y, int s, int frame, int wd);
  void drawSpriteChopped(int x, int y, int s, int frame, int wd, int repeat_at, int alpha = 255);
  void drawSpriteRepeatingX(int x, int y, int s, int frame, int wd);

  SIFSprite sprites[MAX_SPRITES];

private:
  Surface *_spritesheets[MAX_SPRITESHEETS];
  int _num_spritesheets = 0;
  int _num_sprites = 0;
  std::vector<std::string> _sheetfiles;

  void _offset_by_draw_points();
  void _expand_single_dir_sprites();
  void _create_slope_boxes();
  bool _load_sif(const std::string &fname);
  void _loadSheetIfNeeded(int sheetno);
};
}; // namespace Graphics
}; // namespace NXE

#endif
