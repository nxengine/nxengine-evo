
#ifndef _SPRITES_H
#define _SPRITES_H

#define MAX_SPRITESHEETS 64
#define MAX_SPRITES 512

#include "../siflib/sif.h"
#include "nxsurface.h"
extern SIFSprite sprites[MAX_SPRITES];

#include "../nx_math.h"

namespace Sprites
{
bool Init();
void Close();
void FlushSheets();

void BlitSprite(int x, int y, int s, int frame, uint8_t dir, int xoff, int yoff, int wd, int ht, int alpha = 255);
void draw_in_batch(bool enabled);

void draw_sprite(int x, int y, int s, int frame = 0, uint8_t dir = 0);
void draw_sprite_at_dp(int x, int y, int s, int frame = 0, uint8_t dir = 0);
void draw_sprite_clipped(int x, int y, int s, int frame, uint8_t dir, int clipx1, int clipx2, int clipy1, int clipy2);
void draw_sprite_clip_width(int x, int y, int s, int frame, int wd);
void draw_sprite_chopped(int x, int y, int s, int frame, int wd, int repeat_at, int alpha = 255);
void draw_sprite_repeating_x(int x, int y, int s, int frame, int wd);

RectI get_sprite_rect(int x, int y, int s, int frame = 0, uint8_t dir = 0);

NXSurface *get_spritesheet(int sheetno);
int create_spritesheet(int wd, int ht);
}; // namespace Sprites

#endif
