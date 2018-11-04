
#ifndef _PLAYER_H
#define _PLAYER_H

#include "ai/weapons/whimstar.h"
#include "input.h"
#include "object.h"
#include "p_arms.h"

#include <vector>

#define MAX_INVENTORY 42

class Player : public Object
{
public:
  virtual ~Player();

  // current physics constants (change when you go underwater, etc)
  int fallspeed, walkspeed;
  int fallaccel, jumpfallaccel;
  int walkaccel, jumpwalkaccel;
  int decelspeed;
  int jumpvelocity;

  // attributes player is touching: water etc
  uint32_t touchattr;

  int airleft;      // remaining air (is scaled x10)
  int airshowtimer; // used for blinking "100" when coming out of water

  bool walking, lastwalking;
  int walkanimframe;
  int walkanimtimer;

  bool jumping; // 1 if jump physics are enabled

  uint8_t look;             // 0 = not looking, UP = looking up, DOWN = looking down
  uint8_t lookscroll;       // which way the screen should scroll because of a look
  uint8_t lookscroll_timer; // delays a moment before scrolling when player pushes UP/DOWN

  bool inputs_locked; // if 1, the controls are turned off (player can't control his sprite)
  bool inputs_locked_lasttime;

  bool dead;    // deactivates player AI...do we really need this?
  bool hide;    // if 1, player and his statusbar is invisible
  bool drowned; // if 1, player drowning code has been activated
  // if 1, player is invisible and controls unresponsive
  // but statusbar still shown.
  bool disabled;

  bool lookaway; // 1 makes him look away from screen (eg when you push DOWN)

  int inventory[MAX_INVENTORY]; // list of inventory items we have
  int ninventory;               // how many objects are in inventory[]

  Object *riding;     // if non-null, he is "riding" this object (used for solidity)
  Object *lastriding; // for thud sound
  // if non-null then this points at an object for which ->riding cannot be set.
  // This is used when a platform you are riding tries to push you into the ceiling
  // and so the engine deals with it by having you fall through the platform instead.
  Object *cannotride;
  // object if any that caused player's blocku to be set
  Object *bopped_object;

  // <UNI mode--used with Ironhead battle
  int movementmode;

  // how much fuel you have left in the Booster for this jump
  int boosterfuel;
  int booststate; // one of BoosterDir enum
  int lastbooststate;
  bool hitwhileboosting; // bit of a hack, to make hurthop work

  // controls "+xx" floattext numbers when you gain XP
  FloatText *XPText;

  Weapon weapons[WPN_COUNT];
  std::vector<int> wpnOrder;
  int curWeapon;

  int maxHealth; // max health

  int hurt_time;         // timer for blinks after getting hit
  bool hurt_flash_state; // if 1, player is invisible because he's blinking

  // when displaying the water bubble (air tank item) this is the frame shown
  uint8_t water_shield_frame;
  uint8_t water_shield_timer;

  // items equipped
  uint16_t equipmask;
  WhimsicalStar whimstar;

  // additional "block" points placed further in than the real block points.
  // if these make it inside a block the player is "pushed" out of the block.
  // they can get inside the block because his R/L points are further out than his
  // D/U points because of his odd shape.
  Point repel_l[8];
  Point repel_r[8];
  Point repel_u[8];
  Point repel_d[8];
  int nrepel_l, nrepel_r, nrepel_u, nrepel_d;
};

extern Player *player;
extern bool pinputs[INPUT_COUNT];
extern bool lastpinputs[INPUT_COUNT];

enum PMoveModes
{
  MOVEMODE_NORMAL = 0,
  MOVEMODE_ZEROG  = 1,
  MOVEMODE_DEBUG  = 2
};

// how much fuel you start out with in the Booster on each jump.
// (for both 0.8 and 2.0)
#define BOOSTER_FUEL_QTY 50

enum BoosterState
{
  BOOST_OFF = 0,
  BOOST_UP,
  BOOST_DOWN,
  BOOST_HOZ,
  BOOST_08
};

#define EQUIP_BOOSTER08 0x01
#define EQUIP_MAP 0x02
#define EQUIP_ARMS_BARRIER 0x04
#define EQUIP_TURBOCHARGE 0x08
#define EQUIP_AIRTANK 0x10
#define EQUIP_BOOSTER20 0x20
#define EQUIP_MIMIGA_MASK 0x40
#define EQUIP_WHIMSTAR 0x80
#define EQUIP_NIKUMARU 0x100

// void AddXP(int xp, bool quiet = false);
// void SubXP(int xp, bool quiet = false);
void hurtplayer(int damage);

void PInitFirstTime();
void InitPlayer(void);
void HandlePlayer(void);
void HandlePlayer_am(void);
void PDoPhysics(void);
void PUpdateInput(void);
void PHandleAttributes(void);
void DoWaterCurrents(void);
void PDoWalking(void);
void PDoFalling(void);
void PDoJumping(void);
void PDoLooking(void);
void PStartBooster(void);
void PDoBooster(void);
void PDoBoosterEnd();
void PBoosterSmokePuff();
void PHandleSolidBrickObjects(void);
void PHandleSolidMushyObjects(void);
void PRunSolidMushy(Object *o);
void killplayer(int script);
void PHandleZeroG(void);
void PInitRepel(void);
void PDoRepel(void);
void PTryActivateScript();
void PDoHurtFlash(void);
void PSelectFrame(void);
void PSelectSprite(void);
void GetSpriteForGun(int wpn, int look, int *spr, int *frame);
void GetPlayerShootPoint(int *x_out, int *y_out);
void DrawPlayer(void);

#endif
