#ifndef _OBJECT_H
#define _OBJECT_H
#include "floattext.h"
#include "siflib/sif.h"
// used in SpawnXP
#define XP_SMALL_AMT 1
#define XP_MED_AMT 5
#define XP_LARGE_AMT 20

class Object
{
public:
  virtual ~Object() {} // REQUIRED for subclasses (e.g. Player)

  void SetType(int type);
  void ChangeType(int type);
  void BringToFront();
  void PushBehind(Object *behind);
  void PushBehind(int objtype);

  // --------------------------------------- hit detection w/ map

  uint32_t GetAttributes(const Point *pointlist, int npoints, int *tile = NULL);

  bool CheckAttribute(const Point *pointlist, int npoints, uint32_t attrmask, int *tile_x = NULL, int *tile_y = NULL);

  bool CheckSolidIntersect(Object *other, const Point *pointlist, int npoints);

  // --------------------------------------- overridden convenience versions of above

  bool CheckAttribute(SIFPointList *points, uint32_t attrmask, int *tile_x = NULL, int *tile_y = NULL)
  {
    return CheckAttribute(&points->point[0], points->count, attrmask, tile_x, tile_y);
  }

  uint32_t GetAttributes(SIFPointList *points, int *tile = NULL)
  {
    return GetAttributes(&points->point[0], points->count, tile);
  }

  bool CheckSolidIntersect(Object *other, SIFPointList *points)
  {
    return CheckSolidIntersect(other, &points->point[0], points->count);
  }

  // ---------------------------------------

  void UpdateBlockStates(uint8_t updatemask);
  void SetBlockForSolidBrick(uint8_t updatemask);
  int GetBlockingType();

  // ---------------------------------------

  bool apply_xinertia(int inertia);
  bool apply_yinertia(int inertia);
  void PushPlayerOutOfWay(int xinertia, int yinertia);
  void SnapToGround();

  // ---------------------------------------

  void DealDamage(int dmg, Object *shot = NULL);
  void DealDelayedDamage(int dmg, Object *shot = NULL);
  void Kill();
  void SpawnPowerups();
  void SpawnXP(int amt);

  // ---------------------------------------

  void RunAI();
  void DealContactDamage();
  int GetAttackDirection();

  void OnTick();
  void OnAftermove();
  void OnSpawn();
  void OnDeath();

  // ---------------------------------------

  void animate_seq(int speed, const int *framelist, int nframes);
  void CurlyTargetHere(int mintime = 80, int maxtime = 100);
  void ResetClip();
  void MoveAtDir(int dir, int speed);

  // ---------------------------------------

  void Delete();  // mark for deletion at end of frame
  void Destroy(); // delete immediately
  void DisconnectGamePointers();

  // ---------------------------------------

  int Width();
  int Height();

  int BBoxWidth();
  int BBoxHeight();

  int CenterX();
  int CenterY();
  void SetCenterX(int x);
  void SetCenterY(int y);

  int Left();
  int Right();
  int Top();
  int Bottom();

  int SolidLeft();
  int SolidRight();
  int SolidTop();
  int SolidBottom();

  int ActionPointX();
  int ActionPointY();
  int ActionPoint2X();
  int ActionPoint2Y();
  int DrawPointX();
  int DrawPointY();

  SIFSprite *Sprite();

  // ---------------------------------------

  int type;   // object's type
  int sprite; // sprite # to use with object
  int frame;  // frame of sprite to display

  int x, y;
  int xinertia, yinertia;
  uint8_t dir;

  int hp;       // remaining health
  int damaged;  // remaining health
  int damage;   // if != 0 does this much damage to player on touch
  int state;    // AI state
  int substate; // state of current "common/shared" AI routine
  int dirparam; // for ANP's that use the dir as an extra parameter: see tsc.cpp: SetCSDir

  // if the object has FLAG_SOLID_BRICK set, this is how much damage it does to the
  // player if it runs him into a wall or the ceiling/floor.
  int smushdamage;

  // for enemies' "shaking" effect when hurt
  int shaketime;
  int display_xoff;

  // rising damage points
  FloatText *DamageText;
  // tracks amount of damage dealt quickly, while the objects is still shaking from
  // previous shots. displaying this damage is postponed until the enemy stops shaking.
  int DamageWaiting;

  // for teleportation and other effects
  bool clip_enable;
  int clipx1, clipx2;
  int clipy1, clipy2;

  // for use by AI
  int timer, timer2, timer3;
  int animtimer;
  int animframe;
  int blinktimer;
  int xmark, ymark;
  int xmark2, ymark2;
  uint8_t angle, angleoffset; // used for a few such as skullstep
  int speed;
  int savedhp;

  uint32_t flags;    // NPC flags (from .pxe)
  uint32_t nxflags;  // NXEngine-specific flags
  uint16_t id1, id2; // object identifiers (from .pxe)

  bool onscreen;  // true if currently onscreen (lags 1 frame behind)
  bool invisible; // if true the object will NOT be rendered (but still does collision checking)

// flags which are set if an object is touching a wall, ceiling, or floor
// they're addressable either by the array or individually.
#define BLOCKED_MAP 1
#define BLOCKED_OBJECT 2
  union {
    struct
    {
      uint8_t blockr, blockl, blocku, blockd;
    };
    uint8_t block[4];
  };
  union {
    struct
    {
      uint8_t lastblockr, lastblockl, lastblocku, lastblockd;
    };
    uint8_t lastblock[4];
  };

  // if true, object has been deleted and should be freed before next tick
  bool deleted;

  // the dual-layered linked-list. one list is order of creation is the
  // order AI routines are run in, the other is the z-order and is the
  // order the objects are drawn in.
  Object *prev, *next;
  Object *lower, *higher;

  Object *linkedobject;
  Object *whohit;

  // AI variables used for specific AI functions
  union {
    // for player shots (not enemy shots)
    struct
    {
      int ttl;    // frames left till shot times out; sets range
      int dir;    // direction shot was fired in, LEFT RIGHT UP DOWN.
      int damage; // damage dealt per hit
      int accel;  // acceleration

      int btype; // bullet type
      int level; // weapon level (0, 1, or 2)

      // missile boom spawner used w/ player missiles
      struct
      {
        int range;
        int booms_left;
      } boomspawner;
    } shot;

    struct
    {
      int bultype;
      int nlayers;
      int wave_amt;
    } mgun;

    struct
    { // if 1 on an OBJ_CARRIED_OBJECT then object faces in OPPOSITE direction of carrier
      bool flip;
    } carry;

    struct
    {
      int jumpheight, jumpgrav;
      int falldmg;
      bool canfly;
    } critter;

    struct
    {
      int blockedtime;
      int reachptimer;
      int tryjumptime;
      int impjumptime;
      uint8_t impjump;
      uint8_t look;
      int gunsprite;
      int changedirtimer;
      bool spawned_watershield;
    } curly;

    struct
    {
      bool left_ground;
    } toro;

    struct
    {
      Object *carried_by;
    } sue;

    struct
    {
      bool smoking;
      int smoketimer;
    } balrog;

    struct
    {
      bool fireattack;
    } igor;

    struct
    {
      bool is_horizontal;
      int x1, y1, x2, y2;
    } hvt; // hvtrigger

    struct
    {
      Object *layers[4];
    } cloud;
  };
};

// game objects

#define OBJ_NULL 0

#define OBJ_XP 1
#define OBJ_BEHEMOTH 2
#define OBJ_SMOKE_CLOUD 4
#define OBJ_CRITTER_HOPPING_GREEN 5
#define OBJ_BEETLE_GREEN 6
#define OBJ_BASIL 7
#define OBJ_BEETLE_FREEFLY 8
#define OBJ_BALROG_DROP_IN 9
#define OBJ_IGOR_SHOT 11
#define OBJ_BALROG 12
#define OBJ_FORCEFIELD 13
#define OBJ_SANTAS_KEY 14
#define OBJ_CHEST_CLOSED 15
#define OBJ_SAVE_POINT 16
#define OBJ_RECHARGE 17
#define OBJ_DOOR 18
#define OBJ_BALROG_BUST_IN 19
#define OBJ_COMPUTER 20
#define OBJ_CHEST_OPEN 21
#define OBJ_TELEPORTER 22
#define OBJ_TELEPORTER_LIGHTS 23
#define OBJ_POWER_CRITTER 24
#define OBJ_EGG_ELEVATOR 25 // lift platform in Egg Observation Room
#define OBJ_BAT_CIRCLE 26
#define OBJ_BIG_SPIKE 27
#define OBJ_CRITTER_FLYING 28 // cyan critters from grasstown
#define OBJ_CHTHULU 29
#define OBJ_HERMIT_GUNSMITH 30
#define OBJ_BAT_HANG 31
#define OBJ_LIFE_CAPSULE 32
#define OBJ_BALROG_SHOT_BOUNCE 33
#define OBJ_BED 34
#define OBJ_MANNAN 35
#define OBJ_BALROG_BOSS_FLYING 36
#define OBJ_SIGNPOST 37
#define OBJ_FIREPLACE 38
#define OBJ_SAVE_SIGN 39
#define OBJ_SANTA 40       // NPC Santa
#define OBJ_DOOR_BUSTED 41 // door of shack after balrog breaks it
#define OBJ_SUE 42
#define OBJ_CHALKBOARD 43
#define OBJ_POLISH 44
#define OBJ_POLISHBABY 45
#define OBJ_HVTRIGGER 46
#define OBJ_SANDCROC 47
#define OBJ_SKULLHEAD 49
#define OBJ_SKELETON_SHOT 50 // bones fired by skullhead_carried & skeleton
#define OBJ_CROWWITHSKULL 51
#define OBJ_BLUE_ROBOT_SITTING 52
#define OBJ_SKULLSTEP_FOOT 53
#define OBJ_SKULLSTEP 54
#define OBJ_KAZUMA 55
#define OBJ_BEETLE_BROWN 56
#define OBJ_CROW 57
#define OBJ_GIANT_BEETLE 58 // Basu
#define OBJ_DOOR_ENEMY 59   // attacking exit door from First Cave
#define OBJ_TOROKO 60
#define OBJ_KING 61
#define OBJ_KAZUMA_AT_COMPUTER 62
#define OBJ_TOROKO_SHACK 63         // scared toroko attacking in Shack
#define OBJ_CRITTER_HOPPING_BLUE 64 // from First Cave
#define OBJ_BAT_BLUE 65             // from First Cave
#define OBJ_MISERYS_BUBBLE 66       // misery's bubble that takes Toroko away in Shack
#define OBJ_MISERY_FLOAT 67
#define OBJ_BALROG_BOSS_RUNNING 68
#define OBJ_MUSHROOM_ENEMY 69
#define OBJ_HIDDEN_SPARKLE 70
#define OBJ_CHINFISH 71
#define OBJ_SPRINKLER 72
#define OBJ_WATER_DROPLET 73
#define OBJ_JACK 74 // guards cemetary door in Village
#define OBJ_KANPACHI_FISHING 75
#define OBJ_YAMASHITA_FLOWERS 76  // flowers grown in Yamashita Farm
#define OBJ_YAMASHITA_PAVILION 77 // Sandaime's Pavilion from Yamashita Farm
#define OBJ_POT 78
#define OBJ_MAHIN 79       // from village
#define OBJ_GRAVEKEEPER 80 // man with knife in Mimiga Graveyard
#define OBJ_GIANT_MUSHROOM_ENEMY 81
#define OBJ_MISERY_STAND 82
#define OBJ_NPC_IGOR 83
#define OBJ_GIANT_BEETLE_SHOT 84
#define OBJ_TERMINAL 85
#define OBJ_MISSILE 86
#define OBJ_HEART 87
#define OBJ_BOSS_IGOR 88
#define OBJ_BOSS_IGOR_DEFEATED 89
#define OBJ_CAGE 91
#define OBJ_SUE_AT_COMPUTER 92
#define OBJ_CHACO 93
#define OBJ_GIANT_JELLY 94
#define OBJ_JELLY 95
#define OBJ_FAN_LEFT 96
#define OBJ_FAN_UP 97
#define OBJ_FAN_RIGHT 98
#define OBJ_FAN_DOWN 99
#define OBJ_GRATE 100
#define OBJ_POWERCOMP 101
#define OBJ_POWERSINE 102
#define OBJ_MANNAN_SHOT 103
#define OBJ_FROG 104
#define OBJ_HEY 105         // "Hey!" balloon
#define OBJ_HEY_SPAWNER 106 // creates "Hey!" ballons
#define OBJ_MALCO 107
#define OBJ_BALFROG_SHOT 108
#define OBJ_MALCO_BROKEN 109
#define OBJ_MINIFROG 110
#define OBJ_PTELOUT 111 // player teleporting out (using teleporter)
#define OBJ_PTELIN 112  // player teleporting in (using teleporter)
#define OBJ_PROFESSOR_BOOSTER 113
#define OBJ_PRESS 114
#define OBJ_FRENZIED_MIMIGA 115
#define OBJ_RED_PETALS 116    // scattered around beds
#define OBJ_CURLY 117         // curly (npc)
#define OBJ_CURLY_BOSS 118    // curly (boss)
#define OBJ_TABLECHAIRS 119   // table & chairs
#define OBJ_MIMIGAC1 120      // curly's mimigas
#define OBJ_MIMIGAC2 121      // curly's mimigas
#define OBJ_MIMIGAC_ENEMY 122 // fighting mimigas during 1st boss fight in sand zone
#define OBJ_CURLYBOSS_SHOT 123
#define OBJ_SUNSTONE 124
#define OBJ_HIDDEN_POWERUP 125
#define OBJ_PUPPY_RUN 126
#define OBJ_PUPPY_WAG 130
#define OBJ_PUPPY_SLEEP 131
#define OBJ_PUPPY_BARK 132
#define OBJ_JENKA 133
#define OBJ_ARMADILLO 134
#define OBJ_SKELETON 135
#define OBJ_PUPPY_CARRY 136
#define OBJ_LARGEDOOR_FRAME 137
#define OBJ_LARGEDOOR 138
#define OBJ_DOCTOR 139
#define OBJ_TOROKO_FRENZIED 140
#define OBJ_TOROKO_BLOCK 141
#define OBJ_TOROKO_FLOWER 142
#define OBJ_JENKA_COLLAPSED 143
#define OBJ_TOROKO_TELEPORT_IN 144
#define OBJ_KINGS_SWORD 145 // sticks to linkedobject
#define OBJ_LIGHTNING 146
#define OBJ_CRITTER_SHOOTING_PURPLE 147
#define OBJ_CRITTER_SHOT 148
#define OBJ_BLOCK_MOVEH 149
#define OBJ_NPC_PLAYER 150 // the player as an NPC in cutscenes
#define OBJ_BLUE_ROBOT 151
#define OBJ_SHUTTER_STUCK 152
#define OBJ_GAUDI 153
#define OBJ_GAUDI_DYING 154
#define OBJ_GAUDI_FLYING 155
#define OBJ_GAUDI_FLYING_SHOT 156
#define OBJ_BLOCK_MOVEV 157
#define OBJ_X_FISHY_MISSILE 158 // homing fish projectiles fired by Monster X
#define OBJ_X_DEFEATED 159      // cat that falls out after defeating X
#define OBJ_POOH_BLACK 160      // clinic ghost
#define OBJ_POOH_BLACK_BUBBLE 161
#define OBJ_POOH_BLACK_DYING 162
#define OBJ_DR_GERO 163      // from labyrinth clinic
#define OBJ_NURSE_HASUMI 164 // from labyrinth clinic
#define OBJ_CURLY_COLLAPSED 165
#define OBJ_GAUDI_SHOPKEEP 166       // aka Chaba, at table in labyrinth shop
#define OBJ_BOOSTER_FALLING 167      // professor booster falling after Labyrinth
#define OBJ_BOULDER 168              // large boulder in the Boulder Chamber
#define OBJ_BALROG_BOSS_MISSILES 169 // balrog boss in Boulder Chamber
#define OBJ_BALROG_MISSILE 170
#define OBJ_FIREWHIRR 171
#define OBJ_FIREWHIRR_SHOT 172
#define OBJ_GAUDI_ARMORED 173
#define OBJ_GAUDI_ARMORED_SHOT 174
#define OBJ_GAUDI_EGG 175
#define OBJ_BUYOBUYO_BASE 176
#define OBJ_BUYOBUYO 177
#define OBJ_MINICORE_SHOT 178
#define OBJ_CORE_GHOSTIE 179
#define OBJ_CURLY_AI 180
#define OBJ_CAI_GUN 181
#define OBJ_CAI_MGUN 182
#define OBJ_CAI_WATERSHIELD 183
#define OBJ_SHUTTER_BIG 184
#define OBJ_SHUTTER 185
#define OBJ_ALMOND_LIFT 186
#define OBJ_FUZZ_CORE 187
#define OBJ_FUZZ 188
#define OBJ_ALMOND_ROBOT 190
#define OBJ_WATERLEVEL 191 // controls water level in Almond
#define OBJ_MOTORBIKE 192
#define OBJ_MOTORBIKE_BROKEN 193
#define OBJ_BLUE_ROBOT_REMAINS 194
#define OBJ_GRATING 195
#define OBJ_MOTION_WALL 196 // top & bottom wall during Ironhead battle
#define OBJ_IRONH_FISHY 197 // fishies in IronH battle
#define OBJ_IRONH_SHOT 198
#define OBJ_FAN_DROPLET 199 // air or water current (from fans and Waterway)
#define OBJ_DRAGON_ZOMBIE 200
#define OBJ_DRAGON_ZOMBIE_DEAD 201
#define OBJ_DRAGON_ZOMBIE_SHOT 202
#define OBJ_CRITTER_HOPPING_AQUA 203
#define OBJ_FALLING_SPIKE_SMALL 204
#define OBJ_FALLING_SPIKE_LARGE 205
#define OBJ_COUNTER_BOMB 206
#define OBJ_COUNTER_BOMB_NUMBER 207
#define OBJ_GIANT_BEETLE_2 208
#define OBJ_BEETLE_FREEFLY_2 210
#define OBJ_SPIKE_SMALL 211
#define OBJ_SKY_DRAGON 212 // kazuma's nice dragon (npc)
#define OBJ_NIGHT_SPIRIT 213
#define OBJ_NIGHT_SPIRIT_SHOT 214
#define OBJ_SANDCROC_OSIDE 215 // from outer wall
#define OBJ_PIXEL_CAT 216      // hidden pixel the cat from oside aka "debug kitty"
#define OBJ_ITOH 217
#define OBJ_CORE_BLAST 218
#define OBJ_BUBBLE_SPAWNER 219
#define OBJ_MIMIGA_FARMER_STANDING 220 // from plantation, doesn't move
#define OBJ_MIMIGA_FARMER_WALKING 221  // from plantation, walks back and forth
#define OBJ_JAIL_GRATING 222
#define OBJ_MOMORIN 223
#define OBJ_CHIE 224              // plantation lounge
#define OBJ_MEGANE 225            // plantation lounge (you give him the sprinkler)
#define OBJ_KANPACHI_STANDING 226 // standing version, plantation lounge
#define OBJ_BUCKET 227            // kanpachi's fishing bucket
#define OBJ_DROLL_GUARD 228       // droll that stomps you in Teleporter Room
#define OBJ_RED_FLOWERS_SPROUTS 229
#define OBJ_RED_FLOWERS_BLOOMING 230
#define OBJ_ROCKET 231     // ...that you ride up to Last Cave
#define OBJ_ORANGEBELL 232 // bat swarm from Plantation
#define OBJ_ORANGEBELL_BABY 233
#define OBJ_RED_FLOWERS_PICKED 234
#define OBJ_FLOWERS_PENS1 234 // picked red flowers in lower-right corner of Arthur's House
#define OBJ_MIDORIN 235
#define OBJ_GUNFISH 236
#define OBJ_GUNFISH_SHOT 237
#define OBJ_PROXIMITY_PRESS_HOZ 238 // horizontal-moving presses along rocket path
#define OBJ_MIMIGA_CAGE 239
#define OBJ_MIMIGA_JAILED 240 // mimigas in jail2
#define OBJ_CRITTER_HOPPING_RED 241
#define OBJ_RED_BAT 242
#define OBJ_RED_BAT_SPAWNER 243
#define OBJ_LAVA_DRIP 244
#define OBJ_LAVA_DRIP_SPAWNER 245 // for example at entrance to last cave (hidden)
#define OBJ_PROXIMITY_PRESS_VERT 246
#define OBJ_BOSS_MISERY 247
#define OBJ_MISERY_SHOT 248
#define OBJ_MISERY_PHASE 249 // 2 used in teleport effect
#define OBJ_MISERY_BALL 250
#define OBJ_BLACK_LIGHTNING 251
#define OBJ_MISERY_RING 252
#define OBJ_XP_CAPSULE 253
#define OBJ_HELICOPTER 254 // Balcony helicopter
#define OBJ_HELICOPTER_BLADE 255
#define OBJ_DOCTOR_CROWNED 256
#define OBJ_RED_CRYSTAL 257
#define OBJ_MIMIGA_SLEEPING 258 // plantation lounge
#define OBJ_CURLY_CARRIED 259   // curly being carried via Tow Rope
#define OBJ_MIMIGA_CAGED 260    // from Kings Table
#define OBJ_CHIE_CAGED 261      // from Kings Table
#define OBJ_CHACO_CAGED 262     // from Kings Table
#define OBJ_BOSS_DOCTOR 263
#define OBJ_DOCTOR_SHOT 264 // wave shot
#define OBJ_DOCTOR_SHOT_TRAIL 265
#define OBJ_DOCTOR_BLAST 266         // his explosion of red bouncy shots
#define OBJ_BOSS_DOCTOR_FRENZIED 267 // Muscle Doctor
#define OBJ_IGOR_BALCONY 268         // "igor"-like big Ravil on Balcony
#define OBJ_DOCTOR_BAT 269           // orange bats spawned by Doctor 2
#define OBJ_RED_ENERGY 270           // used by Doctor 2 and Undead Core cutscene
#define OBJ_IRONH_BRICK 271
#define OBJ_BRICK_SPAWNER 272
#define OBJ_DROLL_SHOT 273
#define OBJ_DROLL 274
#define OBJ_PUPPY_ITEMS 275 // wagging puppy such as that gives you life capsule
#define OBJ_RED_DEMON 276   // middle-boss in Last Cave Hidden
#define OBJ_RED_DEMON_SHOT 277
#define OBJ_LITTLE_FAMILY 278 // Little Man, and also used for his family
#define OBJ_FALLING_BLOCK 279 // from Misery and Hell B1
#define OBJ_SUE_TELEPORT_IN 280
#define OBJ_DOCTOR_GHOST 281    // doctor as red energy
#define OBJ_UDMINI_PLATFORM 282 // undead minicore (platforms)
#define OBJ_MISERY_FRENZIED 283 // for Final Battle transformed by DOCTOR_GHOST
#define OBJ_SUE_FRENZIED 284    // for Final Battle transformed by DOCTOR_GHOST
#define OBJ_UD_SPINNER 285
#define OBJ_UD_SPINNER_TRAIL 286
#define OBJ_UD_SMOKE 287
#define OBJ_UD_PELLET 288 // undead core pellet/rock shot
#define OBJ_MISERY_CRITTER 289
#define OBJ_MISERY_BAT 290
#define OBJ_UD_MINICORE_IDLE 291
#define OBJ_QUAKE 292
#define OBJ_UD_BLAST 293
#define OBJ_FALLING_BLOCK_SPAWNER 294
#define OBJ_CLOUD 295
#define OBJ_CLOUD_SPAWNER 296 // clouds from prtFall (ending sequence)
#define OBJ_INTRO_DOCTOR 298  // from intro
#define OBJ_INTRO_KINGS 299   // balrog/misery in bubble (from intro)
#define OBJ_INTRO_CROWN 300   // Demon Crown (from intro)
#define OBJ_MISERY_MISSILE 301
#define OBJ_SCROLL_CONTROLLER 302 // <FON on it and you can do various tricks with controlling the scrolling
#define OBJ_SANTA_CAGED 307       // from Kings Table
#define OBJ_GAUDI_PATIENT 304     // credits
#define OBJ_BABY_PUPPY 305        // credits
#define OBJ_BALROG_MEDIC 306      // credits
#define OBJ_STUMPY 308
#define OBJ_BUTE_FLYING 309
#define OBJ_BUTE_SWORD 310
#define OBJ_BUTE_ARCHER 311
#define OBJ_BUTE_ARROW 312
#define OBJ_MA_PIGNON 313
#define OBJ_MA_PIGNON_ROCK 314
#define OBJ_MA_PIGNON_CLONE 315
#define OBJ_BUTE_DYING 316
#define OBJ_MESA 317
#define OBJ_MESA_DYING 318
#define OBJ_MESA_BLOCK 319
#define OBJ_CURLY_CARRIED_SHOOTING 320
#define OBJ_CCS_GUN 321 // OBJ_CURLY_CARRIED_SHOOTING: her gun
#define OBJ_DELEET 322
#define OBJ_BUTE_FALLING 323
#define OBJ_BUTE_SPAWNER 324
#define OBJ_HP_LIGHTNING 325  // lightning shot & charging from Heavy Press
#define OBJ_TURNING_HUMAN 326 // Itoh/Sue turning human (credits)
#define OBJ_AHCHOO 327
#define OBJ_TRANSMOGRIFIER 328 // credits
#define OBJ_BUILDING_FAN 329   // credits
#define OBJ_ROLLING 330
#define OBJ_BALLOS_BONE 331
#define OBJ_BALLOS_BONE_SPAWNER 332
#define OBJ_BALLOS_TARGET 333
#define OBJ_STRAINING 334 // "straining" effect used in Boulder Chamber cutscene
#define OBJ_IKACHAN 335
#define OBJ_IKACHAN_SPAWNER 336
#define OBJ_NUMAHACHI 337 // in Plantation version of Statue Room
#define OBJ_GREEN_DEVIL 338
#define OBJ_GREEN_DEVIL_SPAWNER 339
#define OBJ_BALLOS_PRIEST 340   // form #1 in Seal Chamber
#define OBJ_BALLOS_SMILE 341    // his closed face that smiles at start of battle
#define OBJ_BALLOS_ROTATOR 342  // 3rd/4th form spiky rotators
#define OBJ_BALLOS_BODY_2 343   // time-limited body?
#define OBJ_BALLOS_EYE_2 344    // time-limited closed eyes?
#define OBJ_BALLOS_SKULL 345    // falling skulls during 3rd form
#define OBJ_BALLOS_PLATFORM 346 // platforms for 4th form
#define OBJ_HOPPY 347
#define OBJ_BALLOS_SPIKES 348
#define OBJ_STATUE_BASE 349 // statues in Statue Room (actionable base)
#define OBJ_BUTE_ARCHER_RED 350
#define OBJ_STATUE 351   // statues in Statue Room
#define OBJ_THE_CAST 352 // friends that surround player at end of credits
#define OBJ_BUTE_SWORD_RED 353
#define OBJ_WALL_COLLAPSER 354   // post-Ballos cutscene
#define OBJ_BALROG_PASSENGER 355 // Player/Curly when rescued from Seal Chamber by Balrog
#define OBJ_BALROG_FLYING 356    // best-ending flying-in-clouds cutscene
#define OBJ_PUPPY_GHOST 357      // Ballos's dog in Corridor/ostep
#define OBJ_MISERY_WIND 358      // seen in best-ending credits
#define OBJ_DROPLET_SPAWNER 359  // spawns small falling water drips from e.g. resevoir
#define OBJ_THANK_YOU 360        // credits
#define OBJ_BALFROG 363

// NXEngine defined objects --
#define OBJ_PLAYER 400
#define OBJ_HEART3 401       // i'm sure there must be...
#define OBJ_MISSILE3 402     // ...a real object number for these???
#define OBJ_LAVA_DROPLET 403 // don't know real objnum for this either

#define OBJ_SKULLHEAD_CARRIED 404 // skullhead when carried by a crow
#define OBJ_BBOX_PUPPET 405       // used to form non-square/irregular bboxes
#define OBJ_SMOKE_DROPPER 406     // used by CMP script command

// Core (Almond)
#define OBJ_CORE_CONTROLLER 410
#define OBJ_CORE_FRONT 411
#define OBJ_CORE_BACK 412
#define OBJ_CORE_MARKER 413
#define OBJ_MINICORE 414

// player bullets
#define OBJ_SHOTS_START 420

#define OBJ_POLAR_SHOT 420           // polar-star type player shot (or machine gun L1)
#define OBJ_MGUN_SPAWNER 421         // object dropped by MGun that handles firing it
#define OBJ_MGUN_LEADER 422          // leading "layer" of Machine Gun L2 & 3
#define OBJ_MGUN_TRAIL 423           // "tail" of Machine Gun L2 & 3
#define OBJ_MGUN_L1_SHOT 424         // level 1 machine gun, AI identical to polar star
#define OBJ_MISSILE_SHOT 425         // missile fired by Missile Launcher
#define OBJ_SUPERMISSILE_SHOT 426    // missile fired by Super Missile Launcher
#define OBJ_MISSILE_BOOM_SPAWNER 427 // spawns boomflashes for missile launcher
#define OBJ_FIREBALL1 428
#define OBJ_FIREBALL23 429
#define OBJ_FIREBALL_TRAIL 430
#define OBJ_BLADE12_SHOT 431
#define OBJ_BLADE3_SHOT 432
#define OBJ_BLADE_SLASH 433
#define OBJ_SNAKE1_SHOT 434
#define OBJ_SNAKE23_SHOT 435
#define OBJ_SNAKE_TRAIL 436
#define OBJ_NEMESIS_SHOT 437
#define OBJ_NEMESIS_SHOT_CURLY 438 // for counting her shots separately in Hell
#define OBJ_BUBBLER12_SHOT 439
#define OBJ_BUBBLER3_SHOT 440
#define OBJ_BUBBLER_SHARP 441 // sharp-looking "launched" shot from Bubbler L3
#define OBJ_SPUR_SHOT 442
#define OBJ_SPUR_TRAIL 443
#define OBJ_WHIMSICAL_STAR 444

#define OBJ_SHOTS_END 449

#define OBJ_OMEGA_BODY 450
#define OBJ_OMEGA_LEG 451
#define OBJ_OMEGA_STRUT 452
#define OBJ_OMEGA_SHOT 453

#define OBJ_IRONH 455

// Monster X
#define OBJ_X_MAINOBJECT 460
#define OBJ_X_BODY 461
#define OBJ_X_TREAD 462
#define OBJ_X_INTERNALS 463
#define OBJ_X_DOOR 464
#define OBJ_X_TARGET 465
#define OBJ_X_FISHY_SPAWNER 466

#define OBJ_SISTERS_HEAD 470
#define OBJ_SISTERS_BODY 471
#define OBJ_SISTERS_MAIN 472

#define OBJ_UDCORE_MAIN 480
#define OBJ_UDCORE_FRONT 481
#define OBJ_UDCORE_BACK 482
#define OBJ_UDCORE_FACE 483
#define OBJ_UDMINI_ROTATOR 484
#define OBJ_UDMINI_BBOX 485

#define OBJ_HEAVY_PRESS 490
#define OBJ_HEAVY_PRESS_SHIELD 491

#define OBJ_BALLOS_MAIN 500
#define OBJ_BALLOS_BODY 501
#define OBJ_BALLOS_EYE 502

#define OBJ_LAST 512

#endif
