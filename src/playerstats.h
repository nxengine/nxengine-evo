#ifndef __PLAYERSTATS_H_
#define __PLAYERSTATS_H_

void AddHealth(int hp);
void AddXP(int xp, bool quiet = false);
void SubXP(int xp, bool quiet = false);
void AddInventory(int item);
void DelInventory(int item);
int FindInventory(int item);
int CheckInventoryList(int item, int *list, int nitems);
void GetWeapon(int wpn, int ammo);
void LoseWeapon(int wpn);
void TradeWeapon(int oldwpn, int newwpn, int ammo);
void AddAmmo(int wpn, int ammo);
void RefillAllAmmo(void);

#endif
