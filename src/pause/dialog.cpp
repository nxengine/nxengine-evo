
#include "../nx.h"
#include "dialog.h"
#include "../game.h"
#include "../graphics/graphics.h"
using namespace Graphics;
#include "../graphics/font.h"
#include "../graphics/sprites.h"

using namespace Sprites;
#include "../TextBox/TextBox.h"
#include "../sound/sound.h"
#include "../input.h"

#include "../autogen/sprites.h"



using namespace Options;
extern std::vector<void*>  optionstack;


#define REPEAT_WAIT	30
#define REPEAT_RATE	4

Dialog::Dialog()
{
    if (widescreen)
    {
        DLG_X = ((SCREEN_WIDTH / 2) - 110);
        DLG_Y = ((SCREEN_HEIGHT / 2) - 100);
        DLG_W = 240;
        DLG_H = 200;
    }
    else
    {
        DLG_X = ((SCREEN_WIDTH / 2) - 88);
        DLG_Y = ((SCREEN_HEIGHT / 2) - 100);
        DLG_W = 190;
        DLG_H = 200;
    }

	onclear = NULL;
	ondismiss = NULL;
	
	fCoords.x = DLG_X;
	fCoords.y = DLG_Y;
	fCoords.w = DLG_W;
	fCoords.h = DLG_H;
	fTextX = (fCoords.x + 48);
	
	fCurSel = 0;
	fNumShown = 0;
	fRepeatTimer = 0;
	
	optionstack.push_back(this);
}

Dialog::~Dialog()
{
	for(unsigned int i=0;i<fItems.size();i++)
		delete fItems.at(i);
	for (std::vector<void*>::iterator it = optionstack.begin() ; it != optionstack.end(); ++it)
	{
	    if (*it==this)
	    {
	        optionstack.erase(it);
	        break;
	    }
	}
}

void Dialog::UpdateSizePos()
{
    if (widescreen)
    {
        DLG_X = ((SCREEN_WIDTH / 2) - 110);
        DLG_Y = ((SCREEN_HEIGHT / 2) - 100);
        DLG_W = 240;
        DLG_H = 200;
    }
    else
    {
        DLG_X = ((SCREEN_WIDTH / 2) - 88);
        DLG_Y = ((SCREEN_HEIGHT / 2) - 100);
        DLG_W = 190;
        DLG_H = 200;
    }

	fCoords.x = ((DLG_W / 2) - (fCoords.w / 2)) + DLG_X;
	fCoords.y = ((DLG_H / 2) - (fCoords.h / 2)) + DLG_Y;
	fTextX = (fCoords.x + 34);
}

void Dialog::SetSize(int w, int h)
{
	fCoords.w = w;
	fCoords.h = h;
	fCoords.x = ((DLG_W / 2) - (w / 2)) + DLG_X;
	fCoords.y = ((DLG_H / 2) - (h / 2)) + DLG_Y;
	fTextX = (fCoords.x + 34);
}

void Dialog::offset(int xd, int yd)
{
	fCoords.x += xd;
	fCoords.y += yd;
	fTextX += xd;
}

/*
void c------------------------------() {}
*/

ODItem *Dialog::AddItem(const char *text, \
						void (*activate)(ODItem *, int), \
						void (*update)(ODItem *),\
						int id, int type)
{
	ODItem *item = new ODItem;
	memset(item, 0, sizeof(ODItem));
	
	strcpy(item->text, text);
	
	item->activate = activate;
	item->update = update;
	item->id = id;
	item->type = type;
	
	fItems.push_back(item);
	
	if (update)
		(*update)(item);
	
	return item;
}

ODItem *Dialog::AddSeparator()
{
	return AddItem("", NULL, NULL, -1, OD_SEPARATOR);
}

ODItem *Dialog::AddDismissalItem(const char *text)
{
	if (!text) text = "Return";
	return AddItem(text, NULL, NULL, -1, OD_DISMISS);
}

/*
void c------------------------------() {}
*/

void Dialog::Draw()
{
	TextBox::DrawFrame(fCoords.x, fCoords.y, fCoords.w, fCoords.h);
	
	int x = fTextX;
	int y = (fCoords.y + GetFontBase());
	for(unsigned int i=0;i<fItems.size();i++)
	{
		ODItem *item = (ODItem *)fItems.at(i);
		
		if (i < (unsigned int)fNumShown)
			DrawItem(x, y, item);
		
		if (i == (unsigned int)fCurSel)
			draw_sprite(x - 16, y+1, SPR_WHIMSICAL_STAR, 1);
		if (item->type == OD_SEPARATOR)
			y += 5;
		else
			y += GetFontHeight();
	}
	
	if (fNumShown < 99)
		fNumShown++;
}

void Dialog::DrawItem(int x, int y, ODItem *item)
{
char text[264];

	strcpy(text, _(item->text).c_str());
	strcat(text, _(item->suffix).c_str());
	
	// comes first because it can trim the text on the left side if needed
	if (item->raligntext[0])
	{
		int rx = (fCoords.x + fCoords.w) - 10;
		rx -= GetFontWidth(_(item->raligntext));
		font_draw(rx, y, _(item->raligntext));
		/*
		// ... ellipses if too long
		int maxx = (rx - 4);
		//FillRect(maxx, 0, maxx, SCREEN_HEIGHT, 0,255,0);
		for(;;)
		{
			int wd = GetFontWidth(text);
			if (x+wd < maxx) break;
			
			int len = strlen(text);
			if (len <= 3) { *text = 0; break; }
			
			text[len-1] = 0;
			text[len-2] = '.';
			text[len-3] = '.';
			text[len-4] = '.';
		}*/
	}
	
	font_draw(x, y, text);
	
	// for key remaps
	if (item->righttext[0])
	{
		font_draw((fCoords.x + fCoords.w) - 62, y, item->righttext);
	}
}


/*
void c------------------------------() {}
*/

void Dialog::RunInput()
{
	if (inputs[UPKEY] || inputs[DOWNKEY])
	{
		int dir = (inputs[DOWNKEY]) ? 1 : -1;
		
		if (!fRepeatTimer)
		{
			fRepeatTimer = (lastinputs[UPKEY] || lastinputs[DOWNKEY]) ? REPEAT_RATE : REPEAT_WAIT;
			sound(SND_MENU_MOVE);
			
			int nitems = fItems.size();
			for(;;)
			{
				fCurSel += dir;
				if (fCurSel < 0) fCurSel = (nitems - 1);
				if (fCurSel>=(int)fItems.size()) fCurSel = 0;
				
				if(fCurSel>=0 && fCurSel<(int)fItems.size())
				{
				  ODItem *item = fItems.at(fCurSel);
				  if (item && item->type != OD_SEPARATOR) break;
				}
			}
		}
		else fRepeatTimer--;
	}
	else fRepeatTimer = 0;
	
	if (buttonjustpushed() || justpushed(RIGHTKEY) || justpushed(LEFTKEY) || justpushed(ENTERKEY))
	{
		int dir = (!inputs[LEFTKEY] || buttonjustpushed() || justpushed(RIGHTKEY) || justpushed(ENTERKEY)) ? 1 : -1;
		if (buttonjustpushed() || justpushed(ENTERKEY)) dir = 0;
		
		ODItem *item = NULL;
		if (fCurSel>=0 && fCurSel<(int)fItems.size())
		    item=fItems.at(fCurSel);
		if (item)
		{
			if (item->type == OD_DISMISS)
			{
				if (dir == 0)
				{
					sound(SND_MENU_MOVE);
					if (ondismiss) (*ondismiss)();
					return;
				}
			}
			else if ((item->type == OD_ACTIVATED) && item->activate && (dir == 0))
			{
				(*item->activate)(item, dir);
				
				if (item->update)
					(*item->update)(item);
			}
			else if ((item->type == OD_CHOICE) && item->activate && (dir != 0))
			{
				(*item->activate)(item, dir);
				
				if (item->update)
					(*item->update)(item);
			}
		}
	}
	
	if (justpushed(ESCKEY))
	{
		sound(SND_MENU_MOVE);
		if (ondismiss) (*ondismiss)();
		return;
//		Dismiss();
//		return;
	}
}

void Dialog::SetSelection(int sel)
{
	if (sel < 0) sel = fItems.size();
	if (sel >= (int)fItems.size()) sel = (fItems.size() - 1);
	
	fCurSel = sel;
}


void Dialog::Dismiss()
{
	delete this;
}

void Dialog::Refresh()
{
	for(unsigned int i=0;i<fItems.size();i++)
	{
	    ODItem* item = fItems.at(i);
		if (item->update)
			(*item->update)(item);
	}
}

void Dialog::Clear()
{
	if (onclear)
		(*onclear)();
	
	for(unsigned int i=0;i<fItems.size();i++)
		delete fItems.at(i);
	
	fItems.clear();
	fNumShown = 0;
	fCurSel = 0;
}

std::vector<ODItem*>& Dialog::Items()
{
    return fItems;
}



