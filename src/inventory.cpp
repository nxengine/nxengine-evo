
// the in-game inventory screen

#include "inventory.h"

#include "autogen/sprites.h"
#include "graphics/Renderer.h"
#include "nx.h"
#include "game.h"
#include "input.h"
#include "p_arms.h"
#include "player.h"
#include "sound/SoundManager.h"
#include "statusbar.h"
#include "tsc.h"

using namespace NXE::Graphics;

#define ARMS_X 10
#define ARMS_Y 8

#define ITEMS_X 10
#define ITEMS_Y 60

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

  // find current weapon and count # items for armssel selector
  inv.armssel.items[0] = 0; // show "no weapon" in case of no weapon
  inv.armssel.nitems   = 0;
  for (auto &i : player->wpnOrder)
  {
    if (!player->weapons[i].hasWeapon)
      continue;

    if (player->curWeapon == i)
      curwpn = inv.armssel.nitems;
    inv.armssel.items[inv.armssel.nitems++] = i;
  }

  inv.armssel.spacing_x  = 40;
  inv.armssel.spacing_y  = 0;
  inv.armssel.sprite     = SPR_SELECTOR_ARMS;
  inv.armssel.sound      = NXE::Sound::SFX::SND_SWITCH_WEAPON;
  inv.armssel.scriptbase = 1000;
  inv.armssel.rowlen     = inv.armssel.nitems;

  // setup itemsel...
  inv.itemsel.nitems   = player->ninventory;
  inv.itemsel.items[0] = 0; // show "no item" in case of no items
  for (i = 0; i < player->ninventory; i++)
    inv.itemsel.items[i] = player->inventory[i];

  inv.itemsel.spacing_x  = Renderer::getInstance()->sprites.sprites[SPR_ITEMIMAGE].w;
  inv.itemsel.spacing_y  = Renderer::getInstance()->sprites.sprites[SPR_ITEMIMAGE].h + 2;
  inv.itemsel.sprite     = SPR_SELECTOR_ITEMS;
  inv.itemsel.sound      = NXE::Sound::SFX::SND_MENU_MOVE;
  inv.itemsel.rowlen     = 6;
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
  { // permanently dim
    selector->flashstate = 1;
    selector->animtimer  = 99; // light up immediately upon becoming active
  }

  if (selector->rowlen)
  {
    xsel = (selector->cursel % selector->rowlen);
    ysel = (selector->cursel / selector->rowlen);
  }
  else
    xsel = ysel = 0;

  selx = x + (xsel * selector->spacing_x);
  sely = y + (ysel * selector->spacing_y);
  Renderer::getInstance()->sprites.drawSprite(selx, sely, selector->sprite, selector->flashstate, 0);
}

void DrawInventory(void)
{
  int inv_x = (Renderer::getInstance()->screenWidth / 2) - (inv.w / 2);
  int inv_y = 8;
  int x, y, i, c;

  // draw the box
  TextBox::DrawFrame(inv_x, inv_y, inv.w, inv.h);

  // - draw the weapons ----
  x = inv_x + ARMS_X;
  y = inv_y + ARMS_Y;
  Renderer::getInstance()->sprites.drawSprite(x, y, SPR_TEXT_ARMS, 0, 0);
  y += Renderer::getInstance()->sprites.sprites[SPR_TEXT_ARMS].h;

  DrawSelector(&inv.armssel, x, y);

  // draw the arms
  for (auto &w : player->wpnOrder)
  {
    if (!player->weapons[w].hasWeapon)
      continue;

    Renderer::getInstance()->sprites.drawSprite(x + 1, y + 1, SPR_ARMSICONS, w, 0);
    DrawWeaponLevel(x + 1, y + 16, w);
    DrawWeaponAmmo(x + 1, y + 16 + 8, w);

    x += inv.armssel.spacing_x;
  }

  // - draw the items ----
  x = inv_x + ITEMS_X;
  y = inv_y + ITEMS_Y;
  Renderer::getInstance()->sprites.drawSprite(x, y, SPR_TEXT_ITEMS, 0, 0);
  y += Renderer::getInstance()->sprites.sprites[SPR_TEXT_ITEMS].h;

  DrawSelector(&inv.itemsel, x, y);

  c = 0;
  for (i = 0; i < inv.itemsel.nitems; i++)
  {
    Renderer::getInstance()->sprites.drawSprite(x, y, SPR_ITEMIMAGE, inv.itemsel.items[i], 0);

    x += inv.itemsel.spacing_x;

    if (++c >= inv.itemsel.rowlen)
    {
      x = inv_x + ITEMS_X;
      y += inv.itemsel.spacing_y;
      c = 0;
    }
  }
}

static void ExitInventory(void)
{
  game.tsc->StopScripts();
  game.setmode(GM_NORMAL);
  player->inputs_locked = false;
}

static void RunSelector(stSelector *selector)
{
  int nrows;
  int currow, curcol;
  char toggle = 0;

  if (inv.lockinput)
  {
    if (game.tsc->GetCurrentScript() == -1)
      inv.lockinput = 0;
    else
      return;
  }

  if (selector->nitems)
  {
    nrows  = (selector->nitems - 1) / selector->rowlen;
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
    { // wrap to end of row
      if (currow < nrows)
        selector->cursel += (selector->rowlen - 1);
      else if (selector->nitems > 0)
        selector->cursel = selector->nitems - 1;
    }
    else
      selector->cursel--;
  }

  if (justpushed(RIGHTKEY))
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(selector->sound);

    // at end of row?
    if (curcol == selector->rowlen - 1 || selector->cursel + 1 >= selector->nitems)
    { // wrap to beginning of row
      selector->cursel = (currow * selector->rowlen);
    }
    else
      selector->cursel++;
  }

  if (justpushed(DOWNKEY))
  {
    // on last row?
    if (currow >= nrows)
      toggle = 1;
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
    if (currow == 0)
      toggle = 1;
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

  if (selector == &inv.armssel) // selecting a weapon
  {
    if (buttonjustpushed() || justpushed(INVENTORYKEY))
    { // select the new weapon
      weapon_slide(LEFT, selector->items[selector->cursel]);
      ExitInventory();
    }
  }
  else // selecting an item
  {
    if (justpushed(ACCEPT_BUTTON))
    { // bring up "more info" or "equip" script for this item
      game.tsc->StartScript(selector->items[selector->cursel] + selector->scriptbase + 1000,
                            TSC::ScriptPages::SP_ARMSITEM);
      inv.lockinput = 1;
    }

    if (justpushed(INVENTORYKEY) || justpushed(DECLINE_BUTTON))
    {
      weapon_slide(LEFT, inv.armssel.items[inv.armssel.cursel]);
      ExitInventory();
    }
  }
}

// can't enter Inven if
//  * script is running
//  * fade is in progress
//  * player is dead

// param is passed as 1 when returning from Map System.
bool inventory_init(int param)
{
  int oldarms = inv.armssel.cursel;
  memset(&inv, 0, sizeof(inv));

  inv.curselector          = &inv.armssel;
  inv.armssel.cursel       = RefreshInventoryScreen();
  inv.curselector->lastsel = -9999; // run the script first time

  // returning from map system?
  if (param == 1)
  {
    inv.curselector = &inv.itemsel;
    inv.armssel.cursel = oldarms;

    // highlight Map System
    for (int i = 0; i < inv.itemsel.nitems; i++)
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
  textbox.Tick();
}
