
#include "nx.h"

#include "playerstats.h"
#include "player.h"
#include "inventory.h"
#include "sound/sound.h"
#include "common/stat.h"
#include "caret.h"
#include "statusbar.h"
#include "game.h"


void AddHealth(int hp)
{
	player->hp += hp;
	if (player->hp > player->maxHealth) player->hp = player->maxHealth;
}

void AddXP(int xp, bool quiet)
{
Weapon *weapon = &player->weapons[player->curWeapon];
bool leveled_up = false;

	weapon->xp += xp;
	
	// leveling up...
	while(weapon->xp > weapon->max_xp[weapon->level])
	{
		if (weapon->level < 2)
		{
			weapon->xp -= weapon->max_xp[weapon->level];
			weapon->level++;
			leveled_up = true;
		}
		else
		{
			weapon->xp = weapon->max_xp[weapon->level];
			break;
		}
	}
	
	statusbar.xpflashcount = 30;
	
	if (player->curWeapon == WPN_SPUR)
		leveled_up = false;
	
	if (!quiet)
	{
		if (!player->hide)
		{
			if (leveled_up)
			{
				sound(SND_LEVEL_UP);
				effect(player->CenterX(), player->CenterY(), EFFECT_LEVELUP);
			}
			else
			{
				sound(SND_GET_XP);
			}
		}
		
		player->XPText->AddQty(xp);
	}
}

void SubXP(int xp, bool quiet)
{
Weapon *weapon = &player->weapons[player->curWeapon];
bool leveled_down = false;

	weapon->xp -= xp;
	
	// leveling down...
	while(weapon->xp < 0)
	{
		if (weapon->level > 0)
		{
			weapon->level--;
			weapon->xp += weapon->max_xp[weapon->level];
			leveled_down = true;
		}
		else
		{
			weapon->xp = 0;
			break;
		}
	}
	
	if (player->curWeapon == WPN_SPUR)
		leveled_down = false;
	
	if (leveled_down && !quiet && !player->hide)
	{
		effect(player->CenterX(), player->CenterY(), EFFECT_LEVELDOWN);
	}
}

/*
void c------------------------------() {}
*/

// add an item to the inventory list (generates an error msg if inventory is full)
void AddInventory(int item)
{
	if (player->ninventory+1 >= MAX_INVENTORY)
		{ staterr("<<<AddInventory: inventory is full>>"); game.running = 0; return; }
	
	sound(SND_GET_ITEM);
	if (FindInventory(item) != -1)
	{
	    return;
	}
	player->inventory[player->ninventory++] = item;
	RefreshInventoryScreen();
}

// remove an item from the inventory list (does nothing if it's not in there)
void DelInventory(int item)
{
int slot;
int i;

	for(;;)
	{
		slot = FindInventory(item);
		if (slot == -1) break;
		
		for(i=slot;i<player->ninventory-1;i++)
		{
			player->inventory[i] = player->inventory[i+1];
		}
		player->ninventory--;
	}
	
	RefreshInventoryScreen();
}

// find which slot an item is in (returns -1 if player does not have it)
int FindInventory(int item)
{
	return CheckInventoryList(item, player->inventory, player->ninventory);
}

// checks if the inventory list given contains the given item.
// if so, returns the index of the item. if not, returns -1.
int CheckInventoryList(int item, int *list, int nitems)
{
int i;

	for(i=0;i<nitems;i++)
		if (list[i] == item) return i;
	
	return -1;
}


/*
void c------------------------------() {}
*/

// AM+ command.
// adds "ammo" ammo to the specified weapons ammo and maxammo,
// and if you don't have it already, gives it to you.
void GetWeapon(int wpn, int ammo)
{
	if (!player->weapons[wpn].hasWeapon)
	{
		player->weapons[wpn].ammo = 0;	// will be filled to full by AddAmmo below
		player->weapons[wpn].maxammo = ammo;
		player->weapons[wpn].level = 0;
		player->weapons[wpn].xp = 0;
		player->weapons[wpn].hasWeapon = true;
		player->curWeapon = wpn;
	}
	else
	{	// missile capacity powerups
		player->weapons[wpn].maxammo += ammo;
	}
	
	AddAmmo(wpn, ammo);
	sound(SND_GET_ITEM);
}

// AM- command. Drops specified weapon.
void LoseWeapon(int wpn)
{
	player->weapons[wpn].hasWeapon = false;
	
	// lost current weapon?
	if (wpn == player->curWeapon)
	{
		// in case he has no weapons left at all
		player->curWeapon = WPN_NONE;
		
		// find a new weapon for him
		for(int i=0;i<WPN_COUNT;i++)
		{
			if (player->weapons[i].hasWeapon)
			{
				player->curWeapon = i;
				break;
			}
		}
	}
}

// TAM command.
void TradeWeapon(int oldwpn, int newwpn, int ammo)
{
int oldcurwpn = player->curWeapon;

	// ammo 0 = no change; used when you get missiles are upgraded to Super Missiles
	if (ammo == 0)
		ammo = player->weapons[oldwpn].maxammo;
	
	GetWeapon(newwpn, ammo);
	LoseWeapon(oldwpn);
	
	// switch to new weapon if the weapon traded was the
	// one we were using. Otherwise, don't change current weapon.
	if (oldwpn == oldcurwpn)
		player->curWeapon = newwpn;
	else
		player->curWeapon = oldcurwpn;
}

// adds "ammo" ammo to the specified weapon, but doesn't go over the limit.
void AddAmmo(int wpn, int ammo)
{
	player->weapons[wpn].ammo += ammo;
	if (player->weapons[wpn].ammo > player->weapons[wpn].maxammo)
		player->weapons[wpn].ammo = player->weapons[wpn].maxammo;
}

// sets all weapons to max ammo. AE+ command.
void RefillAllAmmo(void)
{
	for(int i=0;i<WPN_COUNT;i++)
	{
		if (player->weapons[i].hasWeapon)
			player->weapons[i].ammo = player->weapons[i].maxammo;
	}
}

