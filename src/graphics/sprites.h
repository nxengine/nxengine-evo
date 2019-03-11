
#ifndef _SPRITES_H
#define _SPRITES_H

#define MAX_SPRITESHEETS 64
#define MAX_SPRITES 512

#include "../siflib/sif.h"
#include "Surface.h"
extern SIFSprite sprites[MAX_SPRITES];

namespace Sprites
{
bool init();
void close();
void flushSheets();

void blitSprite(int x, int y, int s, int frame, uint8_t dir, int xoff, int yoff, int wd, int ht, int alpha = 255);

void drawSprite(int x, int y, int s, int frame = 0, uint8_t dir = 0);
void drawSpriteAtDp(int x, int y, int s, int frame = 0, uint8_t dir = 0);
void drawSpriteClipped(int x, int y, int s, int frame, uint8_t dir, int clipx1, int clipx2, int clipy1, int clipy2);
void drawSpriteClipWidth(int x, int y, int s, int frame, int wd);
void drawSpriteChopped(int x, int y, int s, int frame, int wd, int repeat_at, int alpha = 255);
void drawSpriteRepeatingX(int x, int y, int s, int frame, int wd);

NXE::Graphics::Surface *getSpritesheet(int sheetno);
}; // namespace Sprites

#endif
