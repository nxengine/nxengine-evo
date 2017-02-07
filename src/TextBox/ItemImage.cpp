
/*
	The powerup display for textboxes.
	E.g. when you get a life capsule or new weapon and
		 it shows you a picture of it.
*/

#include "ItemImage.h"
#include "../graphics/sprites.h"
using namespace Sprites;
#include "TextBox.h"

#define ITEMBOX_W			76
#define ITEMBOX_H			32

#define ITEMBOX_X			128
#define ITEMBOX_Y			120

/*
void c------------------------------() {}
*/

void TB_ItemImage::ResetState()
{
	fVisible = false;
}

void TB_ItemImage::SetVisible(bool enable)
{
	fVisible = enable;
}

void TB_ItemImage::SetSprite(int sprite, int frame)
{
	fSprite = sprite;
	fFrame = frame;
	fYOffset = 1;
}

/*
void c------------------------------() {}
*/

void TB_ItemImage::Draw(void)
{
	if (!fVisible)
		return;
	
	// animate moving item downwards into box
	int desty = (ITEMBOX_H / 2) - (sprites[fSprite].h / 2);
	if (++fYOffset > desty) fYOffset = desty;
	
	// draw the box frame
	TextBox::DrawFrame(ITEMBOX_X, ITEMBOX_Y, ITEMBOX_W, ITEMBOX_H);
	
	// draw the item
	int x = ITEMBOX_X + ((ITEMBOX_W / 2) - (sprites[fSprite].w / 2));
	if (sprites[fSprite].w == 14) x--;		// hack for ArmsIcons
	
	draw_sprite(x, ITEMBOX_Y + fYOffset, fSprite, fFrame);
}





