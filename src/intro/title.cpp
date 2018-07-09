
#include "../nx.h"
#include "title.h"
#include "../profile.h"
#include "../map.h"
#include "../statusbar.h"
#include "../input.h"
#include "../niku.h"
#include "../graphics/font.h"
#include "../sound/sound.h"
#include "../common/stat.h"
#include "../TextBox/TextBox.h"

#include "../settings.h"
#include "../graphics/graphics.h"
#include "../graphics/sprites.h"
#include "../autogen/sprites.h"
using namespace Graphics;
using namespace Sprites;


// music and character selections for the different Counter times
static struct
{
	uint32_t timetobeat;
	int sprite;
	int songtrack;
	int backdrop;
} titlescreens[] =
{
	{(3*3000),   SPR_CS_SUE,    2,	 2}, // 3 mins	- Sue & Safety, on bkGreen
	{(4*3000),   SPR_CS_KING,   41,	 6}, // 4 mins	- King & White, on bkGray
	{(5*3000),   SPR_CS_TOROKO, 40,	 5}, // 5 mins	- Toroko & Toroko's Theme, on bkMaze
	{(6*3000),   SPR_CS_CURLY,  36,	 4}, // 6 mins	- Curly & Running Hell, on bkGard
	{0xFFFFFFFF, SPR_CS_MYCHAR, 24,	 1}  // default - Quote & Cave Story, on bkBlue
};

// artifical fake "loading" delay between selecting an option and it being executed,
// because it actually doesn't look good if we respond instantly.
#define SELECT_DELAY			30
#define SELECT_LOAD_DELAY		20		// delay when leaving the multisave Load dialog
#define SELECT_MENU_DELAY		8		// delay from Load to load menu

static struct
{
	int sprite;
	int cursel;
	int selframe, seltimer;
	int selchoice, seldelay;
	int kc_pos;
	bool in_multiload;
	
	uint32_t besttime;		// Nikumaru display
} title;


static void draw_title()
{
	map_draw_backdrop();
	
	// top logo
	int tx = (SCREEN_WIDTH / 2) - (sprites[SPR_TITLE].w / 2) - 2;
	draw_sprite(tx, 40, SPR_TITLE);
	
	// draw menu

	int cx = (SCREEN_WIDTH / 2) - 32;
	int cy = (SCREEN_HEIGHT / 2) + 8;

    const char* mymenus[] = {"New game","Load game", "Options", "Quit"};

	TextBox::DrawFrame(cx-32,cy-16,128,80);

	for(int i=0;i<=3;i++)
	{
	    font_draw(cx+10,cy, _(mymenus[i]));
		if (i == title.cursel)
		{
			draw_sprite(cx - 16, cy - 1, title.sprite, title.selframe);
		}
		
		cy += 12;
	}

	// animate character
	if (++title.seltimer > 8)
	{
		title.seltimer = 0;
		if (++title.selframe >= sprites[title.sprite].nframes)
			title.selframe = 0;
	}
	
	// accreditation
	cx = (SCREEN_WIDTH / 2) - (sprites[SPR_PIXEL_FOREVER].w / 2);
	int acc_y = SCREEN_HEIGHT - 48;
	draw_sprite(cx, acc_y, SPR_PIXEL_FOREVER);
	
	// version
	int wd = GetFontWidth(NXVERSION);
	cx = (SCREEN_WIDTH / 2) - (wd / 2);
	font_draw(cx, acc_y + sprites[SPR_PIXEL_FOREVER].h + 4, NXVERSION, 0xf3e298);
	
	// draw Nikumaru display
	if (title.besttime != 0xffffffff)
		niku_draw(title.besttime, true);
}



static int kc_table[] = { UPKEY, UPKEY, DOWNKEY, DOWNKEY,
						  LEFTKEY, RIGHTKEY, LEFTKEY, RIGHTKEY, -1 };

void run_konami_code()
{
	if (justpushed(UPKEY) || justpushed(DOWNKEY) || \
		justpushed(LEFTKEY) || justpushed(RIGHTKEY))
	{
		if (justpushed(kc_table[title.kc_pos]))
		{
			title.kc_pos++;
			if (kc_table[title.kc_pos] == -1)
			{
				sound(SND_MENU_SELECT);
				title.kc_pos = 0;
			}
		}
		else
		{
			title.kc_pos = 0;
		}
	}
}

static void handle_input()
{
	if (justpushed(DOWNKEY))
	{
		sound(SND_MENU_MOVE);
		if (++title.cursel >= 4)
			title.cursel = 0;
	}
	else if (justpushed(UPKEY))
	{
		sound(SND_MENU_MOVE);
		if (--title.cursel < 0)
			title.cursel = 3;
	}
	
	if (justpushed(JUMPKEY) || justpushed(ENTERKEY))
	{
		sound(SND_MENU_SELECT);
		int choice = title.cursel;
		
		// handle case where user selects Load but there is no savefile,
		// or the last_save_file is deleted.
		if (title.cursel == 1)
		{
			if (!ProfileExists(settings->last_save_slot))
			{
				bool foundslot = false;
				for(int i=0;i<MAX_SAVE_SLOTS;i++)
				{
					if (ProfileExists(i))
					{
						stat("Last save file %d missing. Defaulting to %d instead.", settings->last_save_slot, i);
						settings->last_save_slot = i;
						foundslot = true;
					}
				}
				
				// there are no save files. Start a new game instead.
				if (!foundslot)
				{
					stat("No save files found. Starting new game instead.");
					choice = 0;
				}
			}
		}
		
		if (choice == 1 && settings->multisave)
		{
			title.selchoice = 10;
			title.seldelay = SELECT_MENU_DELAY;
		}
		else
		{
			title.selchoice = choice;
			if (choice==0) title.seldelay = SELECT_DELAY;
			else title.seldelay = 1;
//			music(0);
		}
	}
	
	run_konami_code();
}

static void selectoption(int index)
{
	switch(index)
	{
		case 0:		// New
		{
			music(0);
			
			game.switchstage.mapno = NEW_GAME_FROM_MENU;
			game.setmode(GM_NORMAL);
		}
		break;
		
		case 1:		// Load
		{
			music(0);
			
			game.switchstage.mapno = LOAD_GAME_FROM_MENU;
			game.setmode(GM_NORMAL);
		}
		break;

		case 2:		// Options
		{
//			music(0);
			game.pause(GP_OPTIONS);
		}
		break;
		case 3:		// Quit
		{
			music(0);
			game.running = false;
		}
		break;

		case 10:		// Load Menu (multisave)
		{
			textbox.SetVisible(true);
			textbox.SaveSelect.SetVisible(true, SS_LOADING);
			title.in_multiload = true;
		}
		break;
	}
}



bool title_init(int param)
{
	memset(&title, 0, sizeof(title));
	game.switchstage.mapno = TITLE_SCREEN;
	game.switchstage.eventonentry = 0;
	game.showmapnametime = 0;
	textbox.SetVisible(false);
	
	if (niku_load(&title.besttime))
		title.besttime = 0xffffffff;
	
	// select a title screen based on Nikumaru time
	int t;
	for(t=0;;t++)
	{
		if (title.besttime < titlescreens[t].timetobeat || \
			titlescreens[t].timetobeat == 0xffffffff)
		{
			break;
		}
	}
	
	title.sprite = titlescreens[t].sprite;
	music(titlescreens[t].songtrack);
	map_set_backdrop(titlescreens[t].backdrop);
	if (titlescreens[t].backdrop == 9
	|| titlescreens[t].backdrop == 10)
		map.scrolltype = BK_FASTLEFT_LAYERS;
	else
		map.scrolltype = BK_TITLE_LEFT;
	map.motionpos = 0;
	
	if (AnyProfileExists())
		title.cursel = 1;	// Load Game
	else
		title.cursel = 0;	// New Game
	
	return 0;
}

void title_tick()
{
	if (!title.in_multiload)
	{
		if (title.seldelay > 0)
		{
			ClearScreen(BLACK);
			
			title.seldelay--;
			if (!title.seldelay)
				selectoption(title.selchoice);
			
			return;
		}
		
		handle_input();
		draw_title();
	}
	else
	{
		ClearScreen(BLACK);
		
		if (!textbox.SaveSelect.IsVisible())
		{	// selection was made, and settings.last_save_slot is now set appropriately
			
			sound(SND_MENU_SELECT);
			
			textbox.SetVisible(false);
			title.in_multiload = false;
			if (!textbox.SaveSelect.Aborted())
			{
				title.selchoice = 1;
				title.seldelay = SELECT_LOAD_DELAY;
			}
		}
		else
		{
			textbox.Draw();
		}
	}
}


/*
void c------------------------------() {}
*/



