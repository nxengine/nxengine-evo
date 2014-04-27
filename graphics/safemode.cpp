
// a limited and redundant graphics system which allows placing text on the screen
// in the case of startup errors or before the real data files are extracted.
#include <stdarg.h>
#include "../nx.h"
#include "safemode.h"
#include "graphics.h"
#include "font.h"
#include "../input.h"

using namespace safemode;
using namespace Graphics;

static int nexty = 0;
static bool initilized = false;
static SDL_Rect printrect;
static NXColor backcolor(0x20, 0x20, 0x20);

static bool have_status = false;
static SDL_Rect statusrect;

bool safemode::init()
{
	moveto(SM_UPPER_THIRD);
	initilized = true;
	
	return 0;
}

void safemode::close()
{
	if (initilized)
	{
		ClearScreen(BLACK);
		screen->Flip();
		
		clear();
	}
}

/*
void c------------------------------() {}
*/

void safemode::moveto(int y)
{
	switch(y)
	{
		case SM_CENTER:
			nexty = (Graphics::SCREEN_HEIGHT / 2) - (GetFontHeight() / 2);
		break;
		
		case SM_UPPER_THIRD:
			nexty = (Graphics::SCREEN_HEIGHT / 4) - (GetFontHeight() / 2);
		break;
		
		case SM_LOWER_THIRD:
			nexty = (Graphics::SCREEN_HEIGHT / 4) - (GetFontHeight() / 2);
			nexty = (Graphics::SCREEN_HEIGHT - nexty);
		break;
		
		case SM_MIDUPPER_Y:
			nexty = (Graphics::SCREEN_HEIGHT / 2) - (GetFontHeight() / 2);
			nexty -= 32;
		break;
		
		default:
			nexty = y;
		break;
	}
}

bool safemode::print(const char *fmt, ...)
{
va_list ar;
char buffer[128];

	va_start(ar, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ar);
	va_end(ar);
	
	stat("safemode print: '%s'", buffer);
	
	int fontwidth = GetFontWidth(buffer, 0, false);
	int fontheight = GetFontHeight();
	
	if (buffer[0])
	{
		int x = ((Graphics::SCREEN_WIDTH / 2) - (fontwidth / 2));
		
		printrect.x = x;
		printrect.y = nexty;
		printrect.w = fontwidth;
		printrect.h = fontheight;
		
		font_draw(x, nexty, buffer, 0, &greenfont);
		screen->Flip();
	}
	
	SDL_Delay(50);
	
	nexty += (fontheight + 1);
	return 0;
}

void safemode::clear()
{
	ClearScreen(backcolor);
	moveto(SM_UPPER_THIRD);
}


void safemode::run_until_key(bool delay)
{
	stat("run_until_key()");
	uint32_t start = SDL_GetTicks();
	
	last_sdl_action.key = -1;
	do
	{
		input_poll();
		SDL_Delay(50);
		
		if (delay && (SDL_GetTicks() - start) < 500)
			last_sdl_action.key = -1;
	}
	while(last_sdl_action.key == -1);
}


void safemode::status(const char *fmt, ...)
{
va_list ar;
char buffer[128];

	va_start(ar, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ar);
	va_end(ar);
	
	clearstatus();
	moveto(SM_CENTER);
	print("%s", buffer);
	
	statusrect = printrect;
	have_status = true;
}

void safemode::clearstatus()
{
	if (have_status)
	{
		FillRect(statusrect.x, statusrect.y, \
				statusrect.x + (statusrect.w - 1), \
				statusrect.y + (statusrect.h - 1),
				backcolor);
		
		moveto(SM_CENTER);
		have_status = false;
	}
}





