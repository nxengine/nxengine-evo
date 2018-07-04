
#include "nx.h"
#include "map.h"
#include "ai/sym/smoke.h"
#include "tsc.h"
#include "ObjManager.h"
#include "caret.h"
#include "debug.h"
#include "graphics/graphics.h"
#include "graphics/font.h"
#include "graphics/sprites.h"
#include "autogen/sprites.h"
#include "graphics/tileset.h"
using namespace Graphics;
using namespace Sprites;
using namespace Tileset;

#include "common/stat.h"
#include "common/misc.h"
#include "common/json.hpp"
#include "game.h"
#include "player.h"
#include "settings.h"
#include "ResourceManager.h"
#include <fstream>


stMap map;

MapRecord stages[MAX_STAGES];
int num_stages;

#define MAX_BACKDROPS			32
NXSurface *backdrop[MAX_BACKDROPS];

// for FindObject--finding NPC's by ID2
Object *ID2Lookup[65536];

unsigned char tilecode[MAX_TILES];			// tile codes for every tile in current tileset
unsigned int tileattr[MAX_TILES];			// tile attribute bits for every tile in current tileset
unsigned int tilekey[MAX_TILES];			// mapping from tile codes -> tile attributes


unsigned char oob_tile_count;
unsigned int oob_tiles[4];

// load stage "stage_no", this entails loading the map (pxm), enemies (pxe), tileset (pbm),
// tile attributes (pxa), and script (tsc).
bool load_stage(int stage_no)
{
char fname[MAXPATHLEN];

	stat(" >> Entering stage %d: '%s'.", stage_no, stages[stage_no].stagename);
	game.curmap = stage_no;		// do it now so onspawn events will have it
	
	if (use_palette)
	{
		Sprites::FlushSheets();
		map_flush_graphics();
	}

	if (Tileset::Load(stages[stage_no].tileset))
		return 1;
	
	// get the base name of the stage without extension
	const char *mapname = stages[stage_no].filename;
	if (!strcmp(mapname, "lounge")) mapname = "Lounge";
	
	sprintf(fname, "Stage/%s.pxm", mapname);
	if (load_map(ResourceManager::getInstance()->getLocalizedPath(fname))) return 1;
	
	sprintf(fname, "Stage/%s.pxa", tileset_names[stages[stage_no].tileset]);
	if (load_tileattr(ResourceManager::getInstance()->getLocalizedPath(fname))) return 1;
	
	sprintf(fname, "Stage/%s.pxe", mapname);
	if (load_entities(ResourceManager::getInstance()->getLocalizedPath(fname))) return 1;
	
	sprintf(fname, "Stage/%s.tsc", mapname);
	if (!game.tsc->Load(ResourceManager::getInstance()->getLocalizedPath(fname), TSC::ScriptPages::SP_MAP)) return 1;
	map_set_backdrop(stages[stage_no].bg_no);
	map.scrolltype = stages[stage_no].scroll_type;
	map.motionpos = 0;

	// optional metadata
	sprintf(fname, "StageMeta/%s.json", mapname);
	load_meta(ResourceManager::getInstance()->getLocalizedPath(fname));
	
	return 0;
}

/*
void c------------------------------() {}
*/

// load a PXM map
bool load_map(const std::string& fname)
{
FILE *fp;
int x, y;

	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp)
	{
		staterr("load_map: no such file: '%s'", fname.c_str());
		return 1;
	}
	
	if (!fverifystring(fp, "PXM"))
	{
		staterr("load_map: invalid map format: '%s'", fname.c_str());
		return 1;
	}
	
	memset(&map, 0, sizeof(map));
	
	fgetc(fp);
	map.xsize = fgeti(fp);
	map.ysize = fgeti(fp);
	
	if (map.xsize > MAP_MAXSIZEX || map.ysize > MAP_MAXSIZEY)
	{
		staterr("load_map: map is too large -- size %dx%d but max is %dx%d", map.xsize, map.ysize, MAP_MAXSIZEX, MAP_MAXSIZEY);
		fclose(fp);
		return 1;
	}
	else
	{
		stat("load_map: level size %dx%d", map.xsize, map.ysize);
	}
	
	for(y=0;y<map.ysize;y++)
	for(x=0;x<map.xsize;x++)
	{
		map.tiles[x][y] = fgetc(fp);
	}
	
	fclose(fp);
	
	if (widescreen)
	{
        if (map.xsize * TILE_W<SCREEN_WIDTH && map.ysize * TILE_W<SCREEN_HEIGHT) {
            map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 80)) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - (SCREEN_HEIGHT - 16)) - 8) * CSFI;
        } else if (map.xsize * TILE_W<SCREEN_WIDTH) {
            if (map.xsize == 25) { // MazeI
                map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 48)) - 8) * CSFI;
                map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
            } else { // Others
                map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 80)) - 8) * CSFI;
                map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
            }
        } else if (map.ysize * TILE_W<SCREEN_HEIGHT) {
            map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - (SCREEN_HEIGHT - 16)) - 8) * CSFI;
        } else {
            map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
        }
	}
	else
	{
    	map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
    	map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
	}
	
	stat("load_map: '%s' loaded OK! - %dx%d", fname.c_str(), map.xsize, map.ysize);
	return 0;
}

void recalc_map_offsets()
{
	if (widescreen)
	{
        if (map.xsize * TILE_W<SCREEN_WIDTH && map.ysize * TILE_W<SCREEN_HEIGHT) {
            map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 80)) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - (SCREEN_HEIGHT - 16)) - 8) * CSFI;
        } else if (map.xsize * TILE_W<SCREEN_WIDTH) {
            if (map.xsize == 25) { // MazeI
                map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 48)) - 8) * CSFI;
                map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
            } else { // Others
                map.maxxscroll = (((map.xsize * TILE_W) - (SCREEN_WIDTH - 80)) - 8) * CSFI;
                map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
            }
        } else if (map.ysize * TILE_W<SCREEN_HEIGHT) {
            map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - (SCREEN_HEIGHT - 16)) - 8) * CSFI;
        } else {
            map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
            map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
        }
	}
	else
	{
    	map.maxxscroll = (((map.xsize * TILE_W) - SCREEN_WIDTH) - 8) * CSFI;
    	map.maxyscroll = (((map.ysize * TILE_H) - SCREEN_HEIGHT) - 8) * CSFI;
	}

}

// load a PXE (entity list for a map)
bool load_entities(const std::string& fname)
{
FILE *fp;
int i;
int nEntities;

	// gotta destroy all objects before creating new ones
	Objects::DestroyAll(false);
	FloatText::ResetAll();
	
	stat("load_entities: reading in %s", fname.c_str());
	// now we can load in the new objects
	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp)
	{
		staterr("load_entities: no such file: '%s'", fname.c_str());
		return 1;
	}
	
	if (!fverifystring(fp, "PXE"))
	{
		staterr("load_entities: not a PXE: '%s'", fname.c_str());
		return 1;
	}
	
	fgetc(fp);
	nEntities = fgetl(fp);
	
	for(i=0;i<nEntities;i++)
	{
		int x = fgeti(fp);
		int y = fgeti(fp);
		int id1 = fgeti(fp);
		int id2 = fgeti(fp);
		int type = fgeti(fp);
		int flags = fgeti(fp);
		
		int dir = (flags & FLAG_FACES_RIGHT) ? RIGHT : LEFT;
		
		//lprintf(" %d:   [%d, %d]\t id1=%d\t id2=%d   Type %d   flags %04x\n", i, x, y, id1, id2, type, flags);
		
		// most maps have apparently garbage entities--invisible do-nothing objects??
		// i dunno but no point in spawning those...
		if (type || id1 || id2 || flags)
		{
			bool addobject = false;
			
			// check if object is dependent on a flag being set/not set
			if (flags & FLAG_APPEAR_ON_FLAGID)
			{
				if (game.flags[id1])
				{
					addobject = true;
					stat(" -- Appearing object %02d (%s) because flag %d is set", id2, DescribeObjectType(type), id1);
				}
			}
			else if (flags & FLAG_DISAPPEAR_ON_FLAGID)
			{
				if (!game.flags[id1])
				{
					addobject = true;
				}
				else
				{
					stat(" -- Disappearing object %02d (%s) because flag %d is set", id2, DescribeObjectType(type), id1);
				}
			}
			else
			{
				addobject = true;
			}
			
			if (addobject)
			{
				// hack for chests (can we do this elsewhere?)
				if (type == OBJ_CHEST_OPEN) y++;
				// hack for skydragon in Fall end cinematic
				if (type == OBJ_SKY_DRAGON && id2 == 230) y++;
				
				Object *o = CreateObject((x * TILE_W) * CSFI, \
										 (y * TILE_H) * CSFI, type,
										 0, 0, dir, NULL, CF_NO_SPAWN_EVENT);
				
				o->id1 = id1;
				o->id2 = id2;
				o->flags |= flags;
				
				ID2Lookup[o->id2] = o;
				
				// now that it's all set up, execute OnSpawn,
				// since we didn't do it in CreateObject.
				o->OnSpawn();
				if (type == OBJ_MOTION_WALL)
				{
				    stat("spawning extra motion wall");
				    o = CreateObject(((x+22) * TILE_W) * CSFI, \
										 (y * TILE_H) * CSFI, type,
										 0, 0, dir, NULL, CF_NO_SPAWN_EVENT);
				    o->id1 = id1;
				    o->id2 = id2;
				    o->flags |= flags;
				
				    ID2Lookup[o->id2] = o;
				
				    // now that it's all set up, execute OnSpawn,
				    // since we didn't do it in CreateObject.
				    o->OnSpawn();
				}

			}
		}
	}
	
	//stat("load_entities: loaded %d objects", nEntities);
	fclose(fp);
	return 0;
}

/*const int ta[] =
{ 0, TA_SOLID, TA_SOLID, TA_SOLID, TA_SOLID,
  TA_SLOPE_BACK1|TA_FOREGROUND, TA_SLOPE_BACK2|TA_FOREGROUND, TA_SLOPE_FWD1|TA_FOREGROUND, TA_SLOPE_FWD2|TA_FOREGROUND,
  TA_FOREGROUND, 0,0,0, TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND, 0, TA_SOLID, TA_SOLID, TA_FOREGROUND, TA_FOREGROUND,
  TA_SOLID,TA_SOLID,TA_SOLID,TA_SOLID,TA_FOREGROUND,0,0,0,TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,
  0,TA_SOLID,TA_FOREGROUND,TA_DESTROYABLE|TA_SOLID,TA_SOLID,TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,TA_SLOPE_CEIL_BACK1|TA_FOREGROUND,TA_SOLID,TA_SOLID,TA_SLOPE_CEIL_FWD2|TA_FOREGROUND,TA_SLOPE_FWD1|TA_FOREGROUND,TA_SLOPE_FWD2|TA_FOREGROUND,
  TA_FOREGROUND,TA_FOREGROUND,TA_SLOPE_CEIL_FWD1|TA_FOREGROUND,TA_SLOPE_CEIL_FWD2|TA_FOREGROUND,TA_SLOPE_CEIL_BACK1|TA_FOREGROUND,TA_SLOPE_CEIL_BACK2|TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,TA_FOREGROUND,0,0,TA_SOLID,TA_SOLID,TA_FOREGROUND,TA_SOLID,TA_SOLID,
  TA_SOLID,TA_SOLID,TA_FOREGROUND|TA_SLOPE_BACK1,TA_SLOPE_BACK2|TA_FOREGROUND,TA_SLOPE_FWD1|TA_FOREGROUND,TA_SLOPE_FWD2|TA_FOREGROUND,TA_SPIKES,TA_SPIKES,TA_SPIKES,TA_SPIKES,0,TA_SOLID,TA_SOLID,0,TA_SOLID,TA_SOLID,
  0,TA_SOLID,0,TA_SOLID,TA_SOLID,0,0,0,0,0,0,TA_SOLID,TA_SOLID,TA_SOLID,TA_SOLID,TA_SOLID,
  TA_SOLID,TA_FOREGROUND,TA_FOREGROUND,0,0,0,0,0,0,0,0,0,TA_SOLID,TA_SOLID,TA_SOLID,TA_SOLID
};
	memset(tileattr, 0, sizeof(tileattr));
	memcpy(&tileattr, &ta, sizeof(ta));
*/

// loads a pxa (tileattr) file
bool load_tileattr(const std::string& fname)
{
FILE *fp;
int i;
unsigned char tc;

	map.nmotiontiles = 0;
	
	stat("load_pxa: reading in %s", fname.c_str());
	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp)
	{
		staterr("load_pxa: no such file: '%s'", fname.c_str());
		return 1;
	}
	
	for(i=0;i<256;i++)
	{
		tc = fgetc(fp);
		tilecode[i] = tc;
		tileattr[i] = tilekey[tc];
		//stat("Tile %02x   TC %02x    Attr %08x   tilekey[%02x] = %08x", i, tc, tileattr[i], tc, tilekey[tc]);
		
		if (tc == 0x43)	// destroyable block - have to replace graphics
		{
			CopySpriteToTile(SPR_DESTROYABLE, i, 0, 0);
		}
		
		// add water currents to animation list
		if (tileattr[i] & TA_CURRENT)
		{
			map.motiontiles[map.nmotiontiles].tileno = i;
			map.motiontiles[map.nmotiontiles].dir = CVTDir(tc & 3);
			map.motiontiles[map.nmotiontiles].sprite = SPR_WATER_CURRENT;
			
			map.nmotiontiles++;
			stat("Added tile %02x to animation list, tc=%02x", i, tc);
		}
	}
	
	fclose(fp);
	return 0;
}

void load_meta(const std::string& fname)
{
std::ifstream fl;

	oob_tile_count = 0;

	fl.open(widen(fname), std::ifstream::in | std::ifstream::binary);
	if (fl.is_open())
	{
		try
		{
			nlohmann::json metadata_root = nlohmann::json::parse(fl);

			// Load out-of-bounds details.
			if (metadata_root.find("out-of-bounds") != metadata_root.end())
			{
				auto oob = metadata_root.at("out-of-bounds");

				// Save OOB tile IDs for rendering.
				if (oob.is_array())
				{
					oob_tile_count = oob.size();
					if (oob_tile_count == 1 || oob_tile_count == 4)
					{
						stat("load_meta: reading %d out-of-bounds tiles", oob_tile_count);
						int i = 0;
						for (auto it = oob.begin(); it != oob.end(); ++it, i++)
						{
							oob_tiles[i] = *it;
						}
					}
					else
					{
						staterr("load_meta: 'out-of-bounds' tile count can only be 1 or 4, found %d", oob_tile_count);
						oob_tile_count = 0;
					}
				}
				else
				{
					staterr("load_meta: metadata field 'out-of-bounds' must be an array containing one or more tile IDs", fname.c_str());
				}
			}

			stat("load_meta: '%s' finished parsing", fname.c_str());
		}
		catch (nlohmann::json::exception& e)
		{
			staterr("load_meta: JSON parsing error in file '%s': %s", fname.c_str(), e.what());
		}
	}
}

bool load_stages(void)
{
FILE *fp;

	fp = myfopen(widen(ResourceManager::getInstance()->getLocalizedPath("stage.dat")).c_str(), widen("rb").c_str());
	if (!fp)
	{
		staterr("%s(%d): failed to open data/stage.dat", __FILE__, __LINE__);
		num_stages = 0;
		return 1;
	}
	
	num_stages = fgetc(fp);
	for(int i=0;i<num_stages;i++)
		fread(&stages[i], sizeof(MapRecord), 1, fp);
		
	//hack to show nice backdrop in menu, like nicalis
	stages[0].bg_no=9;
	//hack to not show ballos in e_Blcn
	stages[93].bossNo = 0;
	
	fclose(fp);
	
	return 0;
}


bool initmapfirsttime(void)
{
FILE *fp;
int i;

	stat("initmapfirsttime: loading data/tilekey.dat.");
	if (!(fp = myfopen(widen(ResourceManager::getInstance()->getLocalizedPath("tilekey.dat")).c_str(), widen("rb").c_str())))
	{
		staterr("data/tilekey.dat is missing!");
		return 1;
	}
	
	for(i=0;i<256;i++)
		tilekey[i] = fgetl(fp);
	
	fclose(fp);
	return load_stages();
}

void initmap(void)
{
	map_focus(NULL);
	map.parscroll_x = map.parscroll_y = 0;
}

/*
void c------------------------------() {}
*/

// loads a backdrop into memory, if it hasn't already been loaded
static bool LoadBackdropIfNeeded(int backdrop_no)
{
std::string fname;
	// load backdrop now if it hasn't already been loaded
	if (!backdrop[backdrop_no])
	{
		// use chromakey (transparency) on bkwater, all others don't
		bool use_chromakey = (backdrop_no == 8);
		if (widescreen)
		{
		    if (backdrop_no == 9)
		    {
		        fname = "bkMoon480fix.pbm";
		    }
		    else if (backdrop_no == 10)
		    {
		        fname = "bkFog480fix.pbm";
		    }
		    else
		    {
		        fname = std::string(backdrop_names[backdrop_no]) + ".pbm";
		    }
		}
		else
		{
		    fname = std::string(backdrop_names[backdrop_no]) + ".pbm";
		}
		
		backdrop[backdrop_no] = NXSurface::FromFile(ResourceManager::getInstance()->getLocalizedPath(fname), use_chromakey);
		if (!backdrop[backdrop_no])
		{
			staterr("Failed to load backdrop '%s'", fname.c_str());
			return 1;
		}
	}
	
	return 0;
}


// backdrop_no 	- backdrop # to switch to
void map_set_backdrop(int backdrop_no)
{
	if (!LoadBackdropIfNeeded(backdrop_no))
		map.backdrop = backdrop_no;
}


void map_draw_backdrop(void)
{
int x, y;
	if (!backdrop[map.backdrop])
	{
		LoadBackdropIfNeeded(map.backdrop);
		if (!backdrop[map.backdrop])
			return;
	}
	
	switch(map.scrolltype)
	{
		case BK_FIXED:
			map.parscroll_x = 0;
			map.parscroll_y = 0;
		break;
		
		case BK_FOLLOWFG:
			map.parscroll_x = (map.displayed_xscroll / CSFI);
			map.parscroll_y = (map.displayed_yscroll / CSFI);
		break;
		
		case BK_PARALLAX:
			map.parscroll_y = (map.displayed_yscroll / CSFI) / 2;
			map.parscroll_x = (map.displayed_xscroll / CSFI) / 2;
			map.parscroll_x %= backdrop[map.backdrop]->Width();
			map.parscroll_y %= backdrop[map.backdrop]->Height();
			if (map.parscroll_x < 0 ) map.parscroll_x = map.parscroll_x * 2;
			if (map.parscroll_y < 0 ) map.parscroll_y = map.parscroll_y * 2;
		break;
		
		case BK_FASTLEFT:		// Ironhead
			if (game.mode == GM_NORMAL)
				map.parscroll_x += 6;
			map.parscroll_y = 0;
		break;
		
		case BK_FASTLEFT_LAYERS:
		case BK_FASTLEFT_LAYERS_NOFALLLEFT:
		{
			DrawFastLeftLayered();
			return;
		}
		break;
		
		case BK_HIDE:
		case BK_HIDE2:
		case BK_HIDE3:
		{
			if (game.curmap == STAGE_KINGS)		// intro cutscene
				ClearScreen(BLACK);
			else
				ClearScreen(DK_BLUE);
		}
		return;
		
		default:
			map.parscroll_x = map.parscroll_y = 0;
			staterr("map_draw_backdrop: unhandled map scrolling type %d", map.scrolltype);
		break;
	}
	int w = backdrop[map.backdrop]->Width();
	int h = backdrop[map.backdrop]->Height();
	
	int mapx = (map.xsize * TILE_W);
	int mapy = (map.ysize * TILE_H);
	// hack for ending Maze map
    if (game.curmap == 74)
    {
        map.parscroll_x-= 16;
        mapx+=64;
    }

    if (game.curmap == 31 && widescreen)
    {
        map.parscroll_y-= 36;
//        mapy+=64;
    }

	for(y=0;y<SCREEN_HEIGHT+map.parscroll_y; y+=h)
	{
		for(x=0;x<SCREEN_WIDTH+map.parscroll_x; x+=w)
		{
		    if ( ((x - map.parscroll_x) < mapx) && ((y - map.parscroll_y) < mapy))
			DrawSurface(backdrop[map.backdrop], x - map.parscroll_x, y - map.parscroll_y);
		}
	}
}

// blit OSide's BK_FASTLEFT_LAYERS
void DrawFastLeftLayered(void)
{
    int layer_ys[] = { 80, 122, 145, 176, 240 };
    if (widescreen)
    {
        layer_ys[4] = 272;
    }

    static const int move_spd[] = { 0,    1,   2,   4,   8 };
    int nlayers = 5;
    int y1, y2;
    int i, x;

	if (--map.parscroll_x <= -(480*SCALE*2))
		map.parscroll_x = 0;
	
	y1 = x = 0;
	// fix for extra height
	if (map.backdrop == 9)
	    ClearScreen(111,156,214);
	else if (map.backdrop == 10 && game.curmap != 64 )
	    ClearScreen(111,107,86);
	for(i=0;i<nlayers;i++)
	{
		y2 = layer_ys[i];
		
		if (i)	// not the static moon layer?
		{
			x = (map.parscroll_x * move_spd[i]) >> 1;
//			x %= SCREEN_WIDTH;
		}
		BlitPatternAcross(backdrop[map.backdrop], x, y1, y1, (y2-y1)+1);
		y1 = (y2 + 1);
	}
	int mapy = map.displayed_yscroll / CSFI;
	if (mapy<0)
		FillRect(0,0,SCREEN_WIDTH, -mapy,0,0,0);
}



void map_flush_graphics()
{
int i;

	for(i=0;i<MAX_BACKDROPS;i++)
	{
		delete backdrop[i];
		backdrop[i] = NULL;
	}
	
	// re-copy star files
	for(i=0;i<256;i++)
	{
		if (tilecode[i] == 0x43)
		{
			CopySpriteToTile(SPR_DESTROYABLE, i, 0, 0);
		}
	}
}


/*
void c------------------------------() {}
*/

// draw rising/falling water from eg Almond etc
void map_drawwaterlevel(void)
{
// water_sfc: 16 tall at 0
// just under: 16 tall at 32
// main tile: 32 tall at 16 (yes, overlapping)
int water_x, water_y;

	if (!map.waterlevelobject)
		return;
	
	water_x = (map.displayed_xscroll / CSFI);
	water_x = ((-water_x) % TILE_W) - TILE_W;
	
	water_y = (map.waterlevelobject->y / CSFI) - (map.displayed_yscroll / CSFI);
	
	// draw the surface and just under the surface
	BlitPatternAcross(backdrop[map.backdrop], water_x, water_y, 0, 16);
	water_y += 16;
	
	BlitPatternAcross(backdrop[map.backdrop], water_x, water_y, 32, 16);
	water_y += 16;
	
	// draw the rest of the pattern all the way down
	while(water_y < (SCREEN_HEIGHT-1))
	{
		BlitPatternAcross(backdrop[map.backdrop], water_x, water_y, 16, 32);
		water_y += 32;
	}
}


// draw the map.
// 	if foreground = TA_FOREGROUND, draws the foreground tile layer.
//  if foreground = 0, draws backdrop and background tiles.
void map_draw(uint8_t foreground)
{
int x, y;
int mapx, mapy;
int blit_x, blit_y, blit_x_start;
int scroll_x, scroll_y;
	
	scroll_x = (map.displayed_xscroll / CSFI);
	scroll_y = (map.displayed_yscroll / CSFI);
	
	mapx = (scroll_x / TILE_W);
	mapy = (scroll_y / TILE_H);
	
	blit_y = -(scroll_y % TILE_H);
	blit_x_start = -(scroll_x % TILE_W);
	
	// MAP_DRAW_EXTRA_Y etc is 1 if resolution is changed to
	// something not a multiple of TILE_H.
	for(y=0; y <= (SCREEN_HEIGHT / TILE_H)+MAP_DRAW_EXTRA_Y; y++)
	{
		blit_x = blit_x_start;
		
		for(x=0; x <= (SCREEN_WIDTH / TILE_W)+MAP_DRAW_EXTRA_X; x++)
		{
			if ( ((mapx+x) >= 0 ) && ((mapy+y) >= 0 ) && ((mapx+x) < map.xsize ) && ((mapy+y) < map.ysize ))
			{
				int t = map.tiles[mapx+x][mapy+y];
				//fixes drawing of debug tiles in Stream and Fall maps
				if( ((game.curmap == 71) && (tilecode[t] == 0x41))
				    ||
				    ((game.curmap == 31) /*&& (tilecode[t] == 0x46)*/)
				) {}
				else
					if ((tileattr[t] & TA_FOREGROUND) == foreground)
						draw_tile(blit_x, blit_y, t);
			}
			blit_x += TILE_W;
		}
		
		blit_y += TILE_H;
	}
}


// draw out-of-bounds tiles, if any.
void map_draw_oob()
{
int x, y;
int mapx, mapy;
int blit_x, blit_y, blit_x_start;
int scroll_x, scroll_y;

	if (oob_tile_count == 0)
		return;

	scroll_x = (map.displayed_xscroll / CSFI);
	scroll_y = (map.displayed_yscroll / CSFI);

	mapx = (scroll_x / TILE_W);
	mapy = (scroll_y / TILE_H);

	blit_y = ((-scroll_y) % TILE_H) - TILE_H;
	blit_x_start = ((-scroll_x) % TILE_W) - TILE_W;

	for(y=0; y <= (SCREEN_HEIGHT / TILE_H)+2; y++)
	{
		blit_x = blit_x_start;

		for(x=0; x <= (SCREEN_WIDTH / TILE_W)+2; x++)
		{
			if (mapx+x <= 0 || mapy+y <= 0 || mapx+x > map.xsize || mapy+y > map.ysize)
			{
				int t = oob_tiles[0];
				if (oob_tile_count == 4)
					t = oob_tiles[abs(mapx+x+1) % 2 + (abs(mapy+y+1) % 2)*2];
				draw_tile(blit_x, blit_y, t);
			}
			blit_x += TILE_W;
		}

		blit_y += TILE_H;
	}
}

/*
void c------------------------------() {}
*/

// map scrolling code
void scroll_normal(void)
{
const int scroll_adj_rate = ((double)0x2000 / (double)map.scrollspeed);
	
	// how many pixels to let player stray from the center of the screen
	// before we start scrolling. high numbers let him reach closer to the edges,
	// low numbers keep him real close to the center.
	#define P_VARY_FROM_CENTER			(64 * CSFI)
	
	if (player->dir == LEFT)
	{
		map.scrollcenter_x -= scroll_adj_rate;
		if (map.scrollcenter_x < -P_VARY_FROM_CENTER)
			map.scrollcenter_x = -P_VARY_FROM_CENTER;
	}
	else
	{
		map.scrollcenter_x += scroll_adj_rate;
		if (map.scrollcenter_x > P_VARY_FROM_CENTER)
			map.scrollcenter_x = P_VARY_FROM_CENTER;
	}
	
	// compute where the map "wants" to be
	map.target_x = (player->CenterX() + map.scrollcenter_x) - ((SCREEN_WIDTH / 2) * CSFI);
	
	// Y scrolling
	if (player->lookscroll == UP)
	{
		map.scrollcenter_y -= scroll_adj_rate;
		if (map.scrollcenter_y < -P_VARY_FROM_CENTER) map.scrollcenter_y = -P_VARY_FROM_CENTER;
	}
	else if (player->lookscroll == DOWN)
	{
		map.scrollcenter_y += scroll_adj_rate;
		if (map.scrollcenter_y > P_VARY_FROM_CENTER) map.scrollcenter_y = P_VARY_FROM_CENTER;
	}
	else
	{
		if (map.scrollcenter_y <= -scroll_adj_rate)
		{
			map.scrollcenter_y += scroll_adj_rate;
		}
		else if (map.scrollcenter_y >= scroll_adj_rate)
		{
			map.scrollcenter_y -= scroll_adj_rate;
		}
	}
	
	map.target_y = (player->CenterY() + map.scrollcenter_y) - ((SCREEN_HEIGHT / 2) * CSFI);
}

void map_scroll_do(void)
{
	bool doing_normal_scroll = false;
	
	if (!map.scroll_locked)
	{
		if (map.focus.has_target)
		{	// FON command
			// this check makes it so if we <FON on an object which
			// gets destroyed, the scroll stays locked at the last known
			// position of the object.
			if (map.focus.target)
			{
				Object *t = map.focus.target;
				
				// Generally we want to focus on the center of the object, not it's UL corner.
				// But a few objects (Cage in mimiga village) have offset drawpoints
				// that affect the positioning of the scene. If the object has a drawpoint,
				// we'll assume it's in an appropriate position, otherwise, we'll try to find
				// the center ourselves.
				if (sprites[t->sprite].frame[t->frame].dir[t->dir].drawpoint.equ(0, 0))
				{
					map.target_x = map.focus.target->CenterX() - ((SCREEN_WIDTH / 2) * CSFI);
					map.target_y = map.focus.target->CenterY() - ((SCREEN_HEIGHT / 2) * CSFI);
				}
				else
				{
					map.target_x = map.focus.target->x - ((SCREEN_WIDTH / 2) * CSFI);
					map.target_y = map.focus.target->y - ((SCREEN_HEIGHT / 2) * CSFI);
				}
			}
		}
		else
		{
			if (!player->hide)
			{
				scroll_normal();
				
				if (!inputs[DEBUG_MOVE_KEY] || !settings->enable_debug_keys)
					doing_normal_scroll = true;
			}
		}
	}
	
	map.real_xscroll += (double)(map.target_x - map.real_xscroll) / (double)map.scrollspeed;
	map.real_yscroll += (double)(map.target_y - map.real_yscroll) / (double)map.scrollspeed;
	
	map.displayed_xscroll = (map.real_xscroll + map.phase_adj);
	map.displayed_yscroll = map.real_yscroll;	// we don't compensate on Y, because player falls > 2 pixels per frame
	
	if (doing_normal_scroll)
	{
		run_phase_compensator();
		//dump_phase_data();
	}
	else
	{
		map.phase_adj -= MAP_PHASE_ADJ_SPEED;
		if (map.phase_adj < 0) map.phase_adj = 0;
	}
	
	map_sanitycheck();
	
	// do quaketime after sanity check so quake works in
	// small levels like Shack.
	if (game.quaketime)
	{
		if (!map.scroll_locked)
		{
			int pushx, pushy;
			
			if (game.megaquaketime)		// Ballos fight
			{
				game.megaquaketime--;
				pushx = random(-5, 5) * CSFI;
				pushy = random(-3, 3) * CSFI;
			}
			else
			{
				pushx = random(-1, 1) * CSFI;
				pushy = random(-1, 1) * CSFI;
			}
			
			map.real_xscroll += pushx;
			map.real_yscroll += pushy;
			map.displayed_xscroll += pushx;
			map.displayed_yscroll += pushy;
		}
		else
		{
			// quake after IronH battle...special case cause we don't
			// want to show the walls of the arena.
			int pushy = random(-0x500, 0x500);
			
			map.real_yscroll += pushy;
			if (map.real_yscroll < 0) map.real_yscroll = 0;
			if (map.real_yscroll > (15 * CSFI)) map.real_yscroll = (15 * CSFI);
			
			map.displayed_yscroll += pushy;
			if (map.displayed_yscroll < 0) map.displayed_yscroll = 0;
			if (map.displayed_yscroll > (15 * CSFI)) map.displayed_yscroll = (15 * CSFI);
		}
		
		game.quaketime--;
	}
}

// this attempts to prevent jitter most visible when the player is walking on a
// long straight stretch. the jitter occurs because map.xscroll and player->x
// tend to be out-of-phase, and thus cross over pixel boundaries at different times.
// what we do here is try to tweak/fudge the displayed xscroll value by up to 512 subpixels
// (1 real pixel), so that it crosses pixel boundaries on exactly the same frame as
// the player does.
void run_phase_compensator(void)
{
	int displayed_phase_offs = (map.displayed_xscroll - player->x) % 512;
	
	if (displayed_phase_offs != 0)
	{
		int phase_offs = abs(map.real_xscroll - player->x) % 512;
		//debug("%d", phase_offs);
		
		// move phase_adj towards phase_offs; phase_offs is how far
		// out of sync we are with the player and so once we reach it
		// we will compensating exactly.
		if (map.phase_adj < phase_offs)
		{
			map.phase_adj += MAP_PHASE_ADJ_SPEED;
			if (map.phase_adj > phase_offs)
				map.phase_adj = phase_offs;
		}
		else
		{
			map.phase_adj -= MAP_PHASE_ADJ_SPEED;
			if (map.phase_adj < phase_offs)
				map.phase_adj = phase_offs;
		}
	}
}

// debug function
void dump_phase_data()
{
	int phase_offs = abs(map.real_xscroll - player->x) % 512;
	int final_phase = abs(map.displayed_xscroll - player->x) % 512;
	debug("phase_offs: %d", phase_offs);
	debug("");
	debug("real xscroll: %d", map.real_xscroll);
	debug("displayed xscroll: %d", map.displayed_xscroll);
	debug("difference: %d", map.real_xscroll - map.displayed_xscroll);
	debug("");
	debug("phase_adj: %d", map.phase_adj);
	debug("final_phase: %d", final_phase);
}

/*
void c------------------------------() {}
*/


// scroll position sanity checking
void map_sanitycheck(void)
{
	#define MAP_BORDER_AMT		(8 * CSFI)
	if (map.real_xscroll < MAP_BORDER_AMT) map.real_xscroll = MAP_BORDER_AMT;
	if (map.real_yscroll < MAP_BORDER_AMT) map.real_yscroll = MAP_BORDER_AMT;
	if (map.real_xscroll > map.maxxscroll) map.real_xscroll = map.maxxscroll;
	if (map.real_yscroll > map.maxyscroll) map.real_yscroll = map.maxyscroll;
	
	if (map.displayed_xscroll < MAP_BORDER_AMT) map.displayed_xscroll = MAP_BORDER_AMT;
	if (map.displayed_yscroll < MAP_BORDER_AMT) map.displayed_yscroll = MAP_BORDER_AMT;
	if (map.displayed_xscroll > map.maxxscroll) map.displayed_xscroll = map.maxxscroll;
	if (map.displayed_yscroll > map.maxyscroll) map.displayed_yscroll = map.maxyscroll;
}


void map_scroll_jump(int x, int y)
{
	map.target_x = x - ((SCREEN_WIDTH / 2) * CSFI);
	map.target_y = y - ((SCREEN_HEIGHT / 2) * CSFI);
	map.real_xscroll = map.target_x;
	map.real_yscroll = map.target_y;
	
	map.displayed_xscroll = map.real_xscroll;
	map.displayed_yscroll = map.real_yscroll;
	map.phase_adj = 0;
	
	map.scrollcenter_x = map.scrollcenter_y = 0;
	map_sanitycheck();
}

// lock the scroll in it's current position. the target position will not change,
// however if the scroll is moved off the target (really only a quake could do this)
// the map will still seek it's old position.
void map_scroll_lock(bool lockstate)
{
	map.scroll_locked = lockstate;
	if (lockstate)
	{	// why do we do this?
		map.real_xscroll = map.target_x;
		map.real_yscroll = map.target_y;
	}
}

// set the map focus and scroll speed.
// if o is specified, focuses on that object.
// if o is NULL, focuses on the player.
void map_focus(Object *o, int spd)
{
	map.focus.target = o;
	map.focus.has_target = (o != NULL);
	
	map.scrollspeed = spd;
	map.scroll_locked = false;
}

/*
void c------------------------------() {}
*/

// change tile at x,y into newtile while optionally spawning smoke clouds and boomflash
void map_ChangeTileWithSmoke(int x, int y, int newtile, int nclouds, bool boomflash, Object *push_behind)
{
	if (x < 0 || y < 0 || x >= map.xsize || y >= map.ysize)
		return;
	
	map.tiles[x][y] = newtile;
	
	int xa = ((x * TILE_W) + (TILE_W / 2)) * CSFI;
	int ya = ((y * TILE_H) + (TILE_H / 2)) * CSFI;
	SmokeXY(xa, ya, nclouds, TILE_W/2, TILE_H/2, push_behind);
	
	if (boomflash)
		effect(xa, ya, EFFECT_BOOMFLASH);
}



const char *map_get_stage_name(int mapno)
{
	if (mapno == STAGE_KINGS)
		return "";//"Studio Pixel Presents";
	
	return stages[mapno].stagename;
}

// show map name for "ticks" ticks
void map_show_map_name()
{
	game.mapname_x = (SCREEN_WIDTH / 2) - (GetFontWidth(_(map_get_stage_name(game.curmap))) / 2);
	game.showmapnametime = 120;
}

void map_draw_map_name(void)
{
	if (game.showmapnametime)
	{
		font_draw(game.mapname_x, 84, _(map_get_stage_name(game.curmap)), 0xFFFFFF, true);
		game.showmapnametime--;
	}
}


// animate all motion tiles
void AnimateMotionTiles(void)
{
int i;
int x_off, y_off;

	for(i=0;i<map.nmotiontiles;i++)
	{
		switch(map.motiontiles[i].dir)
		{
			case LEFT: y_off = 0; x_off = map.motionpos; break;
			case RIGHT: y_off = 0; x_off = (TILE_W - map.motionpos); break;
			
			case UP: x_off = 0; y_off = map.motionpos; break;
			case DOWN: x_off = 0; y_off = (TILE_H - map.motionpos); break;
			
			default: x_off = y_off = 0; break;
		}
		
		CopySpriteToTile(map.motiontiles[i].sprite, map.motiontiles[i].tileno, x_off, y_off);
	}
	
	map.motionpos += 2;
	if (map.motionpos >= TILE_W) map.motionpos = 0;
}


// attempts to find an object with id2 matching the given value else returns NULL
Object *FindObjectByID2(int id2)
{
	Object *result = ID2Lookup[id2];
	
	if (result)
		stat("FindObjectByID2: ID2 %04d found: type %s; coords: (%d, %d)", id2, DescribeObjectType(ID2Lookup[id2]->type), ID2Lookup[id2]->x / CSFI,ID2Lookup[id2]->y / CSFI);
	else
		staterr("FindObjectByID2: no such object %04d", id2);
	
	return result;
}

