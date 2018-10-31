
// the in-game inventory screen

#include "nx.h"
#include "inventory.h"
#include "graphics/graphics.h"
using namespace Graphics;
#include "p_arms.h"
#include "player.h"
#include "graphics/sprites.h"
#include "autogen/sprites.h"
#include "statusbar.h"
#include "game.h"
#include "input.h"
#include "sound/SoundManager.h"
#include "tsc.h"

#define ARMS_X			10
#define ARMS_Y			8

#define ITEMS_X			10
#define ITEMS_Y			60

static stInventory inv;


/*
void c------------------------------() {}
*/


// reload which items & guns are available.
// reset the cursor for the current selector.
// return the slot corresponding to the current weapon.
int RefreshInventoryScreen(void)
{
int i;
int curwpn = 0;
	
	if (game.mode != GM_INVENTORY)
		return 0;
	
	inv.w = 244;
	inv.h = 152;
	inv.x = (SCREEN_WIDTH / 2) - (inv.w / 2);
	if (widescreen)
    	inv.y = 30;
	else
    	inv.y = 8;
	
	
	// find current weapon and count # items for armssel selector
	inv.armssel.items[0] = 0;		// show "no weapon" in case of no weapon
	inv.armssel.nitems = 0;
	for(i=1;i<WPN_COUNT;i++)
	{
		if (!player->weapons[i].hasWeapon) continue;
		
		if (player->curWeapon == i) curwpn = inv.armssel.nitems;
		inv.armssel.items[inv.armssel.nitems++] = i;
	}
	
	inv.armssel.spacing_x = 40;
	inv.armssel.spacing_y = 0;
	inv.armssel.sprite = SPR_SELECTOR_ARMS;
	inv.armssel.sound = NXE::Sound::SFX::SND_SWITCH_WEAPON;
	inv.armssel.scriptbase = 1000;
	inv.armssel.rowlen = inv.armssel.nitems;
	
	// setup itemsel...
	inv.itemsel.nitems = player->ninventory;
	inv.itemsel.items[0] = 0;		// show "no item" in case of no items
	for(i=0;i<player->ninventory;i++)
		inv.itemsel.items[i] = player->inventory[i];
	
	inv.itemsel.spacing_x = sprites[SPR_ITEMIMAGE].w;
	inv.itemsel.spacing_y = sprites[SPR_ITEMIMAGE].h + 2;
	inv.itemsel.sprite = SPR_SELECTOR_ITEMS;
	inv.itemsel.sound = NXE::Sound::SFX::SND_MENU_MOVE;
	inv.itemsel.rowlen = 6;
	inv.itemsel.scriptbase = 5000;
	
	inv.curselector->cursel = 0;
	// after an item has been used don't bring up the script of whatever item
	// the selector is moved to
	inv.curselector->lastsel = inv.curselector->cursel;
	return curwpn;
}

void UnlockInventoryInput(void)
{
	if (inv.lockinput)
		inv.lockinput = false;
}

/*
void c------------------------------() {}
*/


static void DrawSelector(stSelector *selector, int x, int y)
{
int selx, sely;
int xsel, ysel;

	if (selector == inv.curselector)
	{
		// flash the box
		if (++selector->animtimer > 1)
		{
			selector->animtimer = 0;
			selector->flashstate ^= 1;
		}
	}
	else
	{	// permanently dim
		selector->flashstate = 1;
		selector->animtimer = 99;		// light up immediately upon becoming active
	}
	
	if (selector->rowlen)
	{
		xsel = (selector->cursel % selector->rowlen);
		ysel = (selector->cursel / selector->rowlen);
	}
	else xsel = ysel = 0;
	
	selx = x + (xsel * selector->spacing_x);
	sely = y + (ysel * selector->spacing_y);
	Sprites::draw_sprite(selx, sely, selector->sprite, selector->flashstate, 0);
}


static void DrawInventory(void)
{
int x, y, w, i, c;

	// draw the box
	TextBox::DrawFrame(inv.x, inv.y, inv.w, inv.h);
	
	// - draw the weapons ----
	x = inv.x + ARMS_X;
	y = inv.y + ARMS_Y;
	Sprites::draw_sprite(x, y, SPR_TEXT_ARMS, 0, 0);
	y += sprites[SPR_TEXT_ARMS].h;
	
	DrawSelector(&inv.armssel, x, y);
	
	// draw the arms
	for(w=1;w<WPN_COUNT;w++)
	{
		if (!player->weapons[w].hasWeapon) continue;
		
		Sprites::draw_sprite(x+1, y+1, SPR_ARMSICONS, w, 0);
		DrawWeaponLevel(x+1, y+16, w);
		DrawWeaponAmmo(x+1, y+16+8, w);
		
		x += inv.armssel.spacing_x;
	}
	
	// - draw the items ----
	x = inv.x + ITEMS_X;
	y = inv.y + ITEMS_Y;
	Sprites::draw_sprite(x, y, SPR_TEXT_ITEMS, 0, 0);
	y += sprites[SPR_TEXT_ITEMS].h;
	
	DrawSelector(&inv.itemsel, x, y);
	
	c = 0;
	for(i=0;i<inv.itemsel.nitems;i++)
	{
		Sprites::draw_sprite(x, y, SPR_ITEMIMAGE, inv.itemsel.items[i], 0);
		
		x += inv.itemsel.spacing_x;
		
		if (++c >= inv.itemsel.rowlen)
		{
			x = inv.x + ITEMS_X;
			y += inv.itemsel.spacing_y;
			c = 0;
		}
	}
}

static void ExitInventory(void)
{
	game.tsc->StopScripts();
	game.setmode(GM_NORMAL);
	memset(inputs, 0, sizeof(inputs));
}

static void RunSelector(stSelector *selector)
{
int nrows;
int currow, curcol;
char toggle = 0;

	if (inv.lockinput)
	{
		if (game.tsc->GetCurrentScript()==-1) inv.lockinput = 0;
		else return;
	}
	
	if (selector->nitems)
	{
		nrows = (selector->nitems - 1) / selector->rowlen;
		currow = (selector->cursel / selector->rowlen);
		curcol = (selector->cursel % selector->rowlen);
	}
	else
	{
		nrows = currow = curcol = 0;
	}
	
	if (justpushed(LEFTKEY))
	{
		NXE::Sound::SoundManager::getInstance()->playSfx(selector->sound);
		
		// at beginning of row?
		if (curcol == 0)
		{	// wrap to end of row
			if (currow < nrows)
				selector->cursel += (selector->rowlen - 1);
			else if (selector->nitems > 0)
				selector->cursel = selector->nitems - 1;
		}
		else selector->cursel--;
	}
	
	if (justpushed(RIGHTKEY))
	{
		NXE::Sound::SoundManager::getInstance()->playSfx(selector->sound);
		
		// at end of row?
		if (curcol==selector->rowlen-1 || selector->cursel+1 >= selector->nitems)
		{	// wrap to beginning of row
			selector->cursel = (currow * selector->rowlen);
		}
		else selector->cursel++;
	}
	
	if (justpushed(DOWNKEY))
	{
		// on last row?
		if (currow >= nrows) toggle = 1;
		else
		{
			selector->cursel += selector->rowlen;
			
			// don't go past last item
			if (selector->cursel >= selector->nitems)
				selector->cursel = (selector->nitems - 1);
				
			NXE::Sound::SoundManager::getInstance()->playSfx(selector->sound);
		}
	}
	
	if (justpushed(UPKEY))
	{
		// on top row?
		if (currow == 0) toggle = 1;
		else
		{
			selector->cursel -= selector->rowlen;
			NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
		}
	}
	
	// switch to other selector
	if (toggle)
	{
		if (selector == &inv.itemsel)
		{
			selector = &inv.armssel;
		}
		else
		{
			selector = &inv.itemsel;
		}
		
		inv.curselector = selector;
		
		NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
		selector->lastsel = -9999;
	}
	
	// bring up scripts
	if (selector->cursel != selector->lastsel)
	{
		selector->lastsel = selector->cursel;
		
		game.tsc->StartScript(selector->items[selector->cursel] + selector->scriptbase, TSC::ScriptPages::SP_ARMSITEM);
	}
	
	
	if (selector == &inv.armssel)		// selecting a weapon
	{
		if (buttonjustpushed() || justpushed(INVENTORYKEY))
		{	// select the new weapon
			weapon_slide(LEFT, selector->items[selector->cursel]);
			ExitInventory();
		}
	}
	else									// selecting an item
	{
		if (justpushed(JUMPKEY))
		{	// bring up "more info" or "equip" script for this item
			game.tsc->StartScript(selector->items[selector->cursel] + selector->scriptbase + 1000, TSC::ScriptPages::SP_ARMSITEM);
			inv.lockinput = 1;
		}
		
		if (justpushed(INVENTORYKEY) || justpushed(FIREKEY))
			ExitInventory();
	}
}




// can't enter Inven if
//  * script is running
//  * fade is in progress
//  * player is dead

// param is passed as 1 when returning from Map System.
bool inventory_init(int param)
{
	memset(&inv, 0, sizeof(inv));
	
	inv.curselector = &inv.armssel;
	inv.armssel.cursel = RefreshInventoryScreen();
	inv.curselector->lastsel = -9999;		// run the script first time
	
	// returning from map system?
	if (param == 1)
	{
		inv.curselector = &inv.itemsel;
		
		// highlight Map System
		for(int i=0;i<inv.itemsel.nitems;i++)
		{
			if (inv.itemsel.items[i] == 2)
			{
				inv.curselector->cursel = i;
				// textbox NOT up until they move the selector
				inv.curselector->lastsel = i;
				break;
			}
		}
	}
	
	return 0;
}


void inventory_tick(void)
{
	// run the selectors
	RunSelector(inv.curselector);
	
	// draw
	DrawScene();
	DrawInventory();
	textbox.Draw();
}
