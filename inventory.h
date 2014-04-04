
#ifndef _INVENTORY_H
#define _INVENTORY_H

#define MAXLISTLEN		100

struct stSelector
{
	uint8_t flashstate, animtimer;
	
	int spacing_x, spacing_y;
	int cursel, lastsel;
	int sprite;
	int nitems;
	int sound;
	int rowlen;
	
	int scriptbase;
	int items[MAXLISTLEN];
};

struct stInventory
{
	int x, y, w, h;
	
	stSelector armssel;
	stSelector itemsel;
	stSelector *curselector;
	
	char lockinput;
};


bool inventory_init(int param);
void inventory_tick(void);
int RefreshInventoryScreen(void);
void UnlockInventoryInput(void);
static void DrawInventory(void);
static void RunSelector(stSelector *selector);
static void ExitInventory(void);
static void DrawSelector(stSelector *selector, int x, int y);

enum INVENTORY
{
	ITEM_ARTHURS_KEY = 1,
	ITEM_MAP_SYSTEM,
	ITEM_STANTAS_KEY,
	ITEM_SILVER_LOCKET,
	ITEM_BEAST_FANG,
	ITEM_LIFE_CAPSULE,
	ITEM_ID_CARD,
	ITEM_JELLYFISH_JUICE,
	ITEM_RUSTY_KEY,
	ITEM_GUM_KEY,
	ITEM_GUM_BASE,
	ITEM_CHARCOAL,
	ITEM_EXPLOSIVE,
	ITEM_PUPPY,
	ITEM_LIFE_POT,
	ITEM_CUREALL,
	ITEM_CLINIC_KEY,
	ITEM_BOOSTER08,
	ITEM_ARMS_BARRIER,
	ITEM_TURBOCHARGE,
	ITEM_AIRTANK,
	ITEM_COUNTER,
	ITEM_BOOSTER20,
	ITEM_MIMIGA_MASK,
	ITEM_TELEPORTER_KEY,
	ITEM_SUES_LETTER,
	ITEM_CONTROLLER,
	ITEM_BROKEN_SPRINKLER,
	ITEM_SPRINKLER,
	ITEM_TOW_ROPE,
	ITEM_CLAY_FIGURE_MEDAL,
	ITEM_LITTLE_MAN,
	ITEM_MUSHROOM_BADGE,
	ITEM_MA_PIGNON,
	ITEM_CURLYS_UNDERWEAR,
	ITEM_ALIEN_MEDAL,
	ITEM_CHACOS_LIPSTICK,
	ITEM_WHIMSICAL_STAR,
	ITEM_IRON_BOND
};

#endif
