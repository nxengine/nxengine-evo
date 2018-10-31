
#ifndef _PROFILE_H
#define _PROFILE_H
#include "game.h"
#include "p_arms.h"
#include "player.h"

// how many bytes of data long a profile.dat is.
#define PROFILE_LENGTH 0x604

struct Profile
{
  int stage;
  int songno;
  int px, py, pdir;
  int hp, maxhp, num_whimstars;
  uint32_t equipmask;

  int curWeapon;
  struct
  {
    bool hasWeapon;
    int level;
    int xp;
    int ammo, maxammo;
  } weapons[WPN_COUNT];

  int inventory[MAX_INVENTORY];
  int ninventory;

  bool flags[NUM_GAMEFLAGS];

  struct
  {
    int slotno;
    int scriptno;
  } teleslots[NUM_TELEPORTER_SLOTS];
  int num_teleslots;
};

bool profile_load(const char *pfname, Profile *file);
bool profile_save(const char *pfname, Profile *file);
char *GetProfileName(int num);
bool ProfileExists(int num);
bool AnyProfileExists();

#endif
