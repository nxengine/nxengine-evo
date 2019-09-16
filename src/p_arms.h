
#ifndef _P_ARMS_H
#define _P_ARMS_H
#include "object.h"
// player->weapons[] array
enum
{
  WPN_NONE          = 0,
  WPN_SNAKE         = 1,
  WPN_POLARSTAR     = 2,
  WPN_FIREBALL      = 3,
  WPN_MGUN          = 4,
  WPN_MISSILE       = 5,
  WPN_BUBBLER       = 7,
  WPN_BLADE         = 9,
  WPN_SUPER_MISSILE = 10,
  WPN_NEMESIS       = 12,
  WPN_SPUR          = 13,

  WPN_COUNT = 14
};

// stored inside player structure
struct Weapon
{
  bool hasWeapon;    // true if player has this weapon
  int xp, max_xp[3]; // current XP, & max XP per level
  uint8_t level;     // current level (0=L1 1=L2 2=L3)
  int ammo;          // current ammo (0 = n/a)
  int maxammo;       // max ammo (0 = unlimited)

  // for rapid fire weapons. if firerate = 0, must push for each shot.
  int firetimer;
  int firerate[3];

  // for recharging weapons
  int rechargetimer;
  int rechargerate[3];

  // for charged-shot weapons (Spur)
  int chargetimer;

  void SetFireRate(int l1, int l2, int l3)
  {
    firerate[0] = l1;
    firerate[1] = l2;
    firerate[2] = l3;
  }

  void SetRechargeRate(int l1, int l2, int l3)
  {
    rechargerate[0] = l1;
    rechargerate[1] = l2;
    rechargerate[2] = l3;
  }
};

// shot types for SetupBullet.
// matches the order of bullet_table.
enum
{
  B_PSTAR_L1,
  B_PSTAR_L2,
  B_PSTAR_L3,

  B_MGUN_L1,
  B_MGUN_L2,
  B_MGUN_L2P2,
  B_MGUN_L2P3,

  B_MGUN_L3,
  B_MGUN_L3P2,
  B_MGUN_L3P3,
  B_MGUN_L3P4,
  B_MGUN_L3P5,

  B_MISSILE_L1,
  B_MISSILE_L2,
  B_MISSILE_L3,

  B_SUPER_MISSILE_L1,
  B_SUPER_MISSILE_L2,
  B_SUPER_MISSILE_L3,

  B_FIREBALL1,
  B_FIREBALL2,
  B_FIREBALL3,

  B_BLADE_L1,
  B_BLADE_L2,
  B_BLADE_L3,

  B_SNAKE_L1,
  B_SNAKE_L2,
  B_SNAKE_L3,

  B_NEMESIS_L1,
  B_NEMESIS_L2,
  B_NEMESIS_L3,

  B_BUBBLER_L1,
  B_BUBBLER_L2,
  B_BUBBLER_L3,

  B_SPUR_L1,
  B_SPUR_L2,
  B_SPUR_L3,

  B_CURLYS_NEMESIS,

  B_LAST
};

void PResetWeapons();
void PDoWeapons(void);
void FireWeapon(void);
void RunWeapon(bool firing);
void SetupBullet(Object *shot, int x, int y, int btype, int dir);
void FireLevel23MGun(int x, int y, int level, int dir);
void PMgunFly(bool up);

#endif
