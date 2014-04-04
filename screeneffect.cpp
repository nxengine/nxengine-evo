
#include "nx.h"
#include "screeneffect.h"
#include "sound/sound.h"
#include "graphics/graphics.h"
#include "graphics/sprites.h"
using namespace Graphics;
using namespace Sprites;
#include "map.h"

SE_FlashScreen flashscreen;
SE_Starflash starflash;
SE_Fade fade;

/*
void c------------------------------() {}
*/

// FlashScreen simply flashes the screen white several times,
// and is used in various places, such as when Misery casts spells.
void SE_FlashScreen::Start(void)
{
	timer = -1;
	flashes_left = 5;
	flashstate = true;
	
	enabled = true;
}

void SE_FlashScreen::Draw(void)
{
	if (++timer >= 2)
	{
		timer = 0;
		flashstate ^= 1;
		
		if (!flashstate)
		{
			if (--flashes_left <= 0)
				enabled = false;
		}
	}
	
	if (flashstate)
		ClearScreen(0xff, 0xff, 0xff);
}

/*
void c------------------------------() {}
*/

// Starflash is a full-screen white explosion in the shape of a '+',
// used when some bosses are defeated.

void SE_Starflash::Start(int x, int y)
{
	sound(SND_EXPLOSION1);
	enabled = true;
	state = 0;
	centerx = x;
	centery = y;
	size = speed = 0;
}

void SE_Starflash::Draw(void)
{
SE_Starflash * const &star = this;
int scr_x1, scr_y1, scr_x2, scr_y2;
int rel_x, rel_y;

	if (state == 0)
	{	// flash getting bigger
		star->speed += (1 << CSF);
		star->size += star->speed;
	}
	else
	{	// flash getting smaller
		star->size -= (star->size >> 3);
		
		if (star->size < 255)
		{
			enabled = false;
			return;
		}
	}
	
	// draw the flash
	rel_x = (star->centerx - map.displayed_xscroll);
	rel_y = (star->centery - map.displayed_yscroll);
	
	// draw a horizontal bar
	scr_y1 = (rel_y - star->size) >> CSF;
	scr_y2 = (rel_y + star->size) >> CSF;
	FillRect(0, scr_y1, SCREEN_WIDTH, scr_y2, 255, 255, 255);
	
	if (star->state == 0)
	{
		// draw a vertical bar
		scr_x1 = (rel_x - starflash.size) >> CSF;
		scr_x2 = (rel_x + starflash.size) >> CSF;
		FillRect(scr_x1, 0, scr_x2, SCREEN_HEIGHT, 255, 255, 255);
		
		// once it's big enough, switch to making it smaller
		if (star->size > (1280<<CSF))
		{
			star->size = (SCREEN_HEIGHT << CSF);
			star->state = 1;
		}
	}
}

/*
void c------------------------------() {}
*/

#define FADE_LAST_FRAME		15
	
// Fade is the fade-in/out used on every stage transistion/TRA.
// Unlike other effects, it is drawn underneath the textboxes and Nikumaru counter,
// and so isn't drawn from ScreenEffects::Draw().

SE_Fade::SE_Fade()
{
	state = FS_NO_FADE;
	enabled = false;
}

// start a fade in or out.
// fadedir: either FADE_IN or FADE_OUT
// sweepdir: direction to "sweep" the fade
// spr: the sprite to use for the fading
void SE_Fade::Start(int fadedir, int sweepdir, int spr)
{
	if (fadedir == FADE_OUT)
	{
		// when fading out, the directions are reversed
		switch(sweepdir)
		{
			case FADE_LEFT:  sweepdir = FADE_RIGHT; break;
			case FADE_RIGHT: sweepdir = FADE_LEFT; break;
			case FADE_UP:    sweepdir = FADE_DOWN; break;
			case FADE_DOWN:  sweepdir = FADE_UP; break;
		}
	}
	
	state = FS_FADING;
	enabled = true;
	
	fade.sprite = spr;
	
	fade.fadedir = fadedir;
	fade.sweepdir = sweepdir;
	fade.curframe = (fadedir == FADE_OUT) ? -FADE_LAST_FRAME : FADE_LAST_FRAME;
}


void SE_Fade::Draw(void)
{
int x, y;

	#define DRAW_VCOLUMN	\
	{	\
		if (frame >= 0)				\
		{							\
			if (frame > FADE_LAST_FRAME) frame = FADE_LAST_FRAME;	\
			\
			for(y=0;y<SCREEN_HEIGHT;y+=16)							\
				draw_sprite(x, y, fade.sprite, frame);		\
		}		\
	}
	
	#define DRAW_HROW	\
	{	\
		if (frame >= 0)				\
		{							\
			if (frame > FADE_LAST_FRAME) frame = FADE_LAST_FRAME;	\
			\
			for(x=0;x<SCREEN_WIDTH;x+=16)							\
				draw_sprite(x, y, fade.sprite, frame);		\
		}		\
	}
	
	if (state == FS_NO_FADE)
	{
		return;
	}
	else if (state == FS_FADED_OUT)
	{
		ClearScreen(DK_BLUE);
		return;
	}
	
	int frame = fade.curframe;
	switch(fade.sweepdir)
	{
		case FADE_RIGHT:for(x=0;x<SCREEN_WIDTH;x+=16)	   	{ DRAW_VCOLUMN; frame++; }	break;
		case FADE_LEFT: for(x=SCREEN_WIDTH-1;x>=-16;x-=16)  { DRAW_VCOLUMN; frame++; }	break;
		case FADE_DOWN: for(y=0;y<SCREEN_HEIGHT;y+=16)	  	{ DRAW_HROW; frame++; }	break;
		case FADE_UP: 	for(y=SCREEN_HEIGHT-1;y>=-16;y-=16) { DRAW_HROW; frame++; }	break;
		
		case FADE_CENTER:
		{
			int startframe = fade.curframe;
			int centerx = (SCREEN_WIDTH/2)-8;
			int centery = (SCREEN_HEIGHT/2)-8;
			
			for(x=0;x<centerx+16;x+=16)
			{
				frame = startframe;
				
				for(y=0;y<centery+16;y+=16)
				{
					if (frame >= 0)
					{
						if (frame > FADE_LAST_FRAME) frame = FADE_LAST_FRAME;
						
						draw_sprite(centerx+x, centery+y, fade.sprite, frame);
						draw_sprite(centerx-x, centery+y, fade.sprite, frame);
						draw_sprite(centerx+x, centery-y, fade.sprite, frame);
						draw_sprite(centerx-x, centery-y, fade.sprite, frame);
					}
					
					frame++;
				}
				
				startframe++;
			}
		}
		break;
	}
	
	if (fade.fadedir == FADE_OUT)
	{
		fade.curframe++;
		if (fade.curframe > FADE_LAST_FRAME)
			state = FS_FADED_OUT;
	}
	else
	{	// fading in--terminate fade when done
		fade.curframe--;
		if (fade.curframe < -20)
		{
			state = FS_NO_FADE;
			enabled = false;
		}
	}
}

void SE_Fade::set_full(int dir)
{
	if (dir == FADE_OUT)
	{
		state = FS_FADED_OUT;
		
		fade.fadedir = FADE_OUT;
		fade.sweepdir = FADE_RIGHT;			// doesn't matter
		fade.curframe = FADE_LAST_FRAME;
		fade.sprite = SPR_FADE_DIAMOND;
	}
	else
	{
		state = FS_NO_FADE;
		enabled = false;
	}
}

int SE_Fade::getstate(void)
{
	return this->state;
}

/*
void c------------------------------() {}
*/

void ScreenEffects::Draw(void)
{
	if (starflash.enabled)
		starflash.Draw();
	
	if (flashscreen.enabled)
		flashscreen.Draw();
}

void ScreenEffects::Stop()
{
	starflash.enabled = false;
	flashscreen.enabled = false;
}
