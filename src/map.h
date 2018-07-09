#ifndef _MAP_H
#define _MAP_H

#include <string>
#include "object.h"
#include "graphics/tileset.h"

#define MAX_MOTION_TILES			20

#define MAP_MAXSIZEX				300
#define MAP_MAXSIZEY				256

#define MAP_PHASE_ADJ_SPEED			64

// this macro returns true if the current stage is spec'd to use the specified NPC spriteset.
#define DoesCurrentStageUseSpriteset(SET) \
	( stages[game.curmap].NPCset1==SET || stages[game.curmap].NPCset2==SET )

#define MAPX(X)		( ( (X) * TILE_W ) * CSFI )
#define MAPY(Y)		( ( (Y) * TILE_H ) * CSFI )

struct stMap
{
	int xsize, ysize;
	int displayed_xscroll, displayed_yscroll;
	int maxxscroll, maxyscroll;
	int parscroll_x, parscroll_y;
	
	int real_xscroll, real_yscroll;
	int phase_adj;
	
	int target_x, target_y;
	
	int scrollcenter_x, scrollcenter_y;
	int scrollspeed;
	bool scroll_locked;
	
	
	uint8_t backdrop;			// backdrop # in use
	int scrolltype;				// backdrop scroll type
	
	Object *waterlevelobject; 	// object controlling water level (for maps with rising water)
	int wlforcestate;			// set by Core--modifies the state of the water level object
	int wlstate;				// set by WaterLevel object--reports it's current state
	
	// for FON, FOM script commands
	struct
	{
		Object *target;
		bool has_target;
	} focus;
	
	// motion tiles used in Waterway and Main Artery--these tiles use
	// a sprite as their image data and pan the sprite across them in a specified direction
	struct
	{
		int tileno;					// tile # to animate
		uint8_t dir;					// direction of scrolling
		int sprite;					// sprite # containing image data
	} motiontiles[MAX_MOTION_TILES];
	int nmotiontiles;
	int motionpos;
	
	unsigned char tiles[MAP_MAXSIZEX][MAP_MAXSIZEY];
};

extern stMap map;

void map_focus(Object *o, int spd = 16);

// background scrolling types
#define BK_FIXED			0		// backdrop does not scroll
#define BK_PARALLAX			1		// bk is parallax scroll
#define BK_FOLLOWFG			2		// scrolls, but is 1:1 with foreground
#define BK_HIDE				3		// draw #000021 blue instead of a graphic
#define BK_HIDE2			4		// identical to BK_HIDE
#define BK_FASTLEFT			5		// fast scroll left, items falling left (ironhead battle)
#define BK_FASTLEFT_LAYERS	6		// fast scroll left w/ layers, items falling left (Outer Wall)
#define BK_FASTLEFT_LAYERS_NOFALLLEFT	7	// fast left w/ layers, but items don't fall left (Balcony)
#define BK_HIDE3			8		// identical to BK_HIDE
#define BK_TITLE_LEFT		1000

#define MAX_TILES			256				// max # of different kinds of tiles per tileset

// these flag constants come from the stage data somewhere I believe
// (don't remember for sure) so they should stay constant.
#define TA_SOLID_PLAYER		0x00001			// solid to player
#define TA_SOLID_NPC		0x00002			// solid to npc's, enemies and enemy shots
#define TA_SOLID_SHOT		0x00004			// solid to player's shots
#define TA_SOLID			(TA_SOLID_PLAYER | TA_SOLID_SHOT | TA_SOLID_NPC)
#define TA_HURTS_PLAYER		0x00010			// this tile hurts the player -10hp
#define TA_FOREGROUND		0x00020			// tile is drawn in front of sprites
#define TA_DESTROYABLE		0x00040			// tile is destroyable if player shoots it
#define TA_WATER			0x00080			// tile is water/underwater

#define TA_CURRENT			0x00100			// blows player (tilecode checked to see which direction)
#define TA_SLOPE			0x00200			// is a slope (the tilecode is checked to see what kind)

extern uint8_t tilecode[MAX_TILES];
extern uint32_t tileattr[MAX_TILES];
extern uint32_t tilekey[256];
extern Object *ID2Lookup[65536];

void AnimateMotionTiles(void);

void map_ChangeTileWithSmoke(int x, int y, int newtile, int nclouds=4, bool boomflash=false, Object *push_behind=NULL);

// map names and info from stage.dat
#include "maprecord.h"

// backdrop and tileset names
#include "stagedata.h"

// needed if resolution is changed to a non-multiple of the tile size
//#if (((SCREEN_WIDTH / TILE_W) * TILE_W) != SCREEN_WIDTH)
	#define MAP_DRAW_EXTRA_X	1
//#else
//	#define MAP_DRAW_EXTRA_X	0
//#endif

//#if (((SCREEN_HEIGHT / TILE_H) * TILE_H) != SCREEN_HEIGHT)
	#define MAP_DRAW_EXTRA_Y	1
//#else
//	#define MAP_DRAW_EXTRA_Y	0
//#endif

bool load_stage(int stage_no);
bool load_map(const std::string& fname);
bool load_entities(const std::string& fname);
bool load_tileattr(const std::string& fname);
void load_meta(const std::string& fname);
bool load_stages(void);
bool initmapfirsttime(void);
void initmap(void);
void map_set_backdrop(int backdrop_no);
void map_draw_backdrop(void);
void DrawFastLeftLayered(void);
void map_flush_graphics();
void map_drawwaterlevel(void);
void map_draw(uint8_t foreground);
void map_draw_oob();
void scroll_normal(void);
void map_scroll_do(void);
void run_phase_compensator(void);
void dump_phase_data();
void map_sanitycheck(void);
void map_scroll_jump(int x, int y);
void map_scroll_lock(bool lockstate);
void map_focus(Object *o, int spd);
void map_ChangeTileWithSmoke(int x, int y, int newtile, int nclouds, bool boomflash, Object *push_behind);
const std::string& map_get_stage_name(int mapno);
void map_show_map_name();
void map_draw_map_name(void);
void AnimateMotionTiles(void);
Object *FindObjectByID2(int id2);
void recalc_map_offsets();

#endif
