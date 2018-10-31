
#ifndef _MAPRECORD_H
#define _MAPRECORD_H

#include <cstdint>

#define MAX_STAGES 120
struct MapRecord
{
  char filename[32];
  char stagename[35];

  uint8_t tileset;
  uint8_t bg_no;
  uint8_t scroll_type;
  uint8_t bossNo;
  uint8_t NPCset1;
  uint8_t NPCset2;
};
extern MapRecord stages[MAX_STAGES];
extern int num_stages;

#define STAGE_SAND 10
#define STAGE_START_POINT 13
#define STAGE_IRONH 31
#define STAGE_BOULDER_CHAMBER 44
#define STAGE_MAZE_M 45
#define STAGE_ALMOND 47
#define STAGE_WATERWAY 48
#define STAGE_KINGS_TABLE 65
#define STAGE_HELL1 80
#define STAGE_HELL2 81
#define STAGE_HELL3 82
#define STAGE_HELL4 84
#define STAGE_HELL42 85
#define STAGE_STATUE_CHAMBER 86
#define STAGE_SEAL_CHAMBER 87
#define STAGE_SEAL_CHAMBER_2 92
#define STAGE_CORRIDOR 88
#define STAGE_KINGS 72 // intro

#endif
