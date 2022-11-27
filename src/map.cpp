
#include "map.h"

#include "ObjManager.h"
#include "ai/sym/smoke.h"
#include "autogen/sprites.h"
#include "caret.h"
#include "debug.h"
#include "graphics/Renderer.h"
#include "nx.h"
#include "tsc.h"
using namespace NXE::Graphics;

#include "ResourceManager.h"
#include "common/misc.h"
#include "Utils/Logger.h"
#include "game.h"
#include "player.h"
#include "settings.h"

#include <json.hpp>
#include <fstream>

stMap map;

MapRecord stages[MAX_STAGES];
int num_stages;

#define MAX_BACKDROPS 32
Surface *backdrop[MAX_BACKDROPS];

// for FindObject--finding NPC's by ID2
Object *ID2Lookup[65536];

uint8_t tilecode[MAX_TILES]; // tile codes for every tile in current tileset
uint32_t tileattr[MAX_TILES];  // tile attribute bits for every tile in current tileset
uint32_t tilekey[MAX_TILES];   // mapping from tile codes -> tile attributes

uint8_t oob_tile_count;
uint32_t oob_tiles[4];

// load stage "stage_no", this entails loading the map (pxm), enemies (pxe), tileset (pbm),
// tile attributes (pxa), and script (tsc).
bool load_stage(int stage_no)
{
  char fname[MAXPATHLEN];

  LOG_INFO(" >> Entering stage {}: '{}'.", stage_no, stages[stage_no].stagename);
  game.curmap = stage_no; // do it now so onspawn events will have it

  Renderer::getInstance()->sprites.flushSheets();
  map_flush_graphics();

  if (!Renderer::getInstance()->tileset.load(stages[stage_no].tileset))
    return 1;

  // get the base name of the stage without extension
  const char *mapname = stages[stage_no].filename;
  if (!strcmp(mapname, "lounge"))
    mapname = "Lounge";

  sprintf(fname, "Stage/%s.pxm", mapname);
  if (load_map(ResourceManager::getInstance()->getPath(fname, false)))
    return 1;

  sprintf(fname, "Stage/%s.pxa", tileset_names[stages[stage_no].tileset]);
  if (load_tileattr(ResourceManager::getInstance()->getPath(fname, false)))
    return 1;

  sprintf(fname, "Stage/%s.pxe", mapname);
  if (load_entities(ResourceManager::getInstance()->getPath(fname, false)))
    return 1;

  sprintf(fname, "Stage/%s.tsc", mapname);
  if (!game.tsc->Load(ResourceManager::getInstance()->getPath(fname), TSC::ScriptPages::SP_MAP))
    return 1;
  map_set_backdrop(stages[stage_no].bg_no);
  map.scrolltype = stages[stage_no].scroll_type;
  map.motionpos  = 0;

  // optional metadata
  sprintf(fname, "StageMeta/%s.json", mapname);
  load_meta(ResourceManager::getInstance()->getPath(fname, false));

  return 0;
}

/*
void c------------------------------() {}
*/

// load a PXM map
bool load_map(const std::string &fname)
{
  FILE *fp;
  int x, y;

  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("load_map: no such file: '{}'", fname);
    return 1;
  }

  if (!fverifystring(fp, "PXM"))
  {
    LOG_ERROR("load_map: invalid map format: '{}'", fname);
    return 1;
  }

  memset(&map, 0, sizeof(map));

  fgetc(fp);
  map.xsize = fgeti(fp);
  map.ysize = fgeti(fp);

  if (map.xsize > MAP_MAXSIZEX || map.ysize > MAP_MAXSIZEY)
  {
    LOG_ERROR("load_map: map is too large -- size {}x{} but max is {}x{}", map.xsize, map.ysize, MAP_MAXSIZEX,
            MAP_MAXSIZEY);
    fclose(fp);
    return 1;
  }
  else
  {
    LOG_DEBUG("load_map: level size {}x{}", map.xsize, map.ysize);
  }

  for (y = 0; y < map.ysize; y++)
    for (x = 0; x < map.xsize; x++)
    {
      map.tiles[x][y] = fgetc(fp);
      if (game.curmap == 31 && (y == 0 || y == 15))
      {
        map.tiles[x][y] = 0xF; // block up/down in Main Artery
      }
    }

  fclose(fp);

  recalc_map_offsets();

  LOG_DEBUG("load_map: '{}' loaded OK! - {}x{}", fname, map.xsize, map.ysize);
  return 0;
}

void recalc_map_offsets()
{
  if (Renderer::getInstance()->widescreen)
  {
    if (map.xsize * TILE_W < Renderer::getInstance()->screenWidth)
    {
      map.maxxscroll = (((map.xsize * TILE_W) - Renderer::getInstance()->screenWidth) / 2) * CSFI;
    }
    else
    {
      map.maxxscroll = (((map.xsize * TILE_W) - Renderer::getInstance()->screenWidth) - 8) * CSFI;
    }

    if (map.ysize * TILE_W < Renderer::getInstance()->screenHeight)
    {
      map.maxyscroll = (((map.ysize * TILE_H) - (Renderer::getInstance()->screenHeight - 16)) - 8) * CSFI;
    }
    else
    {
      map.maxyscroll = (((map.ysize * TILE_H) - Renderer::getInstance()->screenHeight) - 8) * CSFI;
    }
  }
  else
  {
    map.maxxscroll = (((map.xsize * TILE_W) - Renderer::getInstance()->screenWidth) - 8) * CSFI;
    map.maxyscroll = (((map.ysize * TILE_H) - Renderer::getInstance()->screenHeight) - 8) * CSFI;
  }
}

// load a PXE (entity list for a map)
bool load_entities(const std::string &fname)
{
  FILE *fp;
  int i;
  int nEntities;

  // gotta destroy all objects before creating new ones
  Objects::DestroyAll(false);
  FloatText::ResetAll();

  LOG_DEBUG("load_entities: reading in {}", fname);
  // now we can load in the new objects
  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("load_entities: no such file: '{}'", fname);
    return 1;
  }

  if (!fverifystring(fp, "PXE"))
  {
    LOG_ERROR("load_entities: not a PXE: '{}'", fname);
    return 1;
  }

  fgetc(fp);
  nEntities = fgetl(fp);

  for (i = 0; i < nEntities; i++)
  {
    int x     = fgeti(fp);
    int y     = fgeti(fp);
    int id1   = fgeti(fp);
    int id2   = fgeti(fp);
    int type  = fgeti(fp);
    int flags = fgeti(fp);

    int dir = (flags & FLAG_FACES_RIGHT) ? RIGHT : LEFT;

    // lprintf(" %d:   [%d, %d]\t id1=%d\t id2=%d   Type %d   flags %04x\n", i, x, y, id1, id2, type, flags);

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
          LOG_DEBUG("Appearing object {:%02d} ({}) because flag {} is set", id2, DescribeObjectType(type), id1);
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
          LOG_DEBUG("Disappearing object {:02d} ({}) because flag {} is set", id2, DescribeObjectType(type), id1);
        }
      }
      else
      {
        addobject = true;
      }

      if (addobject)
      {
        // hack for chests (can we do this elsewhere?)
        if (type == OBJ_CHEST_OPEN && dir == RIGHT)
          y++;
        // hack for skydragon in Fall end cinematic
        if (type == OBJ_SKY_DRAGON && id2 == 230)
          y++;

        // hack for Curly in Almond
        if (game.curmap == 47 && id2 == 301 && !game.flags[822])
          y++;

        Object *o = CreateObject((x * TILE_W) * CSFI, (y * TILE_H) * CSFI, type, 0, 0, dir, NULL, CF_NO_SPAWN_EVENT);

        o->id1 = id1;
        o->id2 = id2;
        o->flags |= flags;

        ID2Lookup[o->id2] = o;

        // now that it's all set up, execute OnSpawn,
        // since we didn't do it in CreateObject.
        o->OnSpawn();
        if (type == OBJ_MOTION_WALL)
        {
          LOG_DEBUG("spawning extra motion wall");
          o = CreateObject(((x + 22) * TILE_W) * CSFI, (y * TILE_H) * CSFI, type, 0, 0, dir, NULL, CF_NO_SPAWN_EVENT);
          o->id1 = id1;
          o->id2 = id2;
          o->flags |= flags;

          ID2Lookup[o->id2] = o;

          // now that it's all set up, execute OnSpawn,
          // since we didn't do it in CreateObject.
          o->OnSpawn();

          LOG_DEBUG("spawning extra motion wall");
          o      = CreateObject(((x)*TILE_W) * CSFI, ((y - TILE_H) * TILE_H) * CSFI, type, 0, 0, dir, NULL,
                           CF_NO_SPAWN_EVENT);
          o->id1 = id1;
          o->id2 = id2;
          o->flags |= flags;

          ID2Lookup[o->id2] = o;

          // now that it's all set up, execute OnSpawn,
          // since we didn't do it in CreateObject.
          o->OnSpawn();

          LOG_DEBUG("spawning extra motion wall");
          o      = CreateObject(((x + 22) * TILE_W) * CSFI, ((y - TILE_H) * TILE_H) * CSFI, type, 0, 0, dir, NULL,
                           CF_NO_SPAWN_EVENT);
          o->id1 = id1;
          o->id2 = id2;
          o->flags |= flags;

          ID2Lookup[o->id2] = o;

          // now that it's all set up, execute OnSpawn,
          // since we didn't do it in CreateObject.
          o->OnSpawn();

          LOG_DEBUG("spawning extra motion wall");
          o      = CreateObject(((x)*TILE_W) * CSFI, ((y + TILE_H) * TILE_H) * CSFI, type, 0, 0, dir, NULL,
                           CF_NO_SPAWN_EVENT);
          o->id1 = id1;
          o->id2 = id2;
          o->flags |= flags;

          ID2Lookup[o->id2] = o;

          // now that it's all set up, execute OnSpawn,
          // since we didn't do it in CreateObject.
          o->OnSpawn();

          LOG_DEBUG("spawning extra motion wall");
          o      = CreateObject(((x + 22) * TILE_W) * CSFI, ((y + TILE_H) * TILE_H) * CSFI, type, 0, 0, dir, NULL,
                           CF_NO_SPAWN_EVENT);
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

  LOG_DEBUG("load_entities: loaded {} objects", nEntities);
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
bool load_tileattr(const std::string &fname)
{
  FILE *fp;
  int i;
  unsigned char tc;

  LOG_DEBUG("load_pxa: reading in {}", fname);
  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("load_pxa: no such file: '{}'", fname);
    return 1;
  }

  for (i = 0; i < 256; i++)
  {
    tc          = fgetc(fp);
    tilecode[i] = tc;
    tileattr[i] = tilekey[tc];
    LOG_TRACE("Tile {:#02x}   TC {:#02x}    Attr {:#08x}   tilekey[{:#02x}] = {:#08x}", i, tc, tileattr[i], tc, tilekey[tc]);

    if (game.curmap == 31 && tc == 0x46)
      tileattr[i] = 0; // remove left/right blockers in Mai Artery
  }

  fclose(fp);
  return 0;
}

void load_meta(const std::string &fname)
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
            LOG_DEBUG("load_meta: reading {} out-of-bounds tiles", oob_tile_count);
            int i = 0;
            for (auto it = oob.begin(); it != oob.end(); ++it, i++)
            {
              oob_tiles[i] = *it;
            }
          }
          else
          {
            LOG_ERROR("load_meta: 'out-of-bounds' tile count can only be 1 or 4, found {}", oob_tile_count);
            oob_tile_count = 0;
          }
        }
        else
        {
          LOG_ERROR("load_meta: metadata field 'out-of-bounds' must be an array containing one or more tile IDs");
        }
      }

      LOG_DEBUG("load_meta: '{}' finished parsing", fname);
    }
    catch (nlohmann::json::exception &e)
    {
      LOG_ERROR("load_meta: JSON parsing error in file '{}': {}", fname, e.what());
    }
  }
}

bool load_stages(void)
{
  FILE *fp;

  fp = myfopen(widen(ResourceManager::getInstance()->getPath("stage.dat")).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("failed to open data/stage.dat");
    num_stages = 0;
    return 1;
  }

  num_stages = fgetc(fp);
  for (int i = 0; i < num_stages; i++)
    fread(&stages[i], sizeof(MapRecord), 1, fp);

  // hack to show nice backdrop in menu, like nicalis
  //	stages[0].bg_no=9;
  // hack to not show ballos in e_Blcn
  stages[93].bossNo = 0;

  fclose(fp);

  return 0;
}

bool initmapfirsttime(void)
{
  FILE *fp;
  int i;

  LOG_INFO("Loading tilekey.dat.");
  if (!(fp
        = myfopen(widen(ResourceManager::getInstance()->getPath("tilekey.dat")).c_str(), widen("rb").c_str())))
  {
    LOG_ERROR("tilekey.dat is missing!");
    return 1;
  }

  for (i = 0; i < 256; i++)
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

    if (Renderer::getInstance()->widescreen
        && (backdrop_no == 9 || backdrop_no == 10 || backdrop_no == 12 || backdrop_no == 13 || backdrop_no == 14))
    {
      fname = std::string(backdrop_names[backdrop_no]) + "480fix.pbm";
    }
    else
    {
      fname = std::string(backdrop_names[backdrop_no]) + ".pbm";
    }

    backdrop[backdrop_no] = Surface::fromFile(ResourceManager::getInstance()->getPath(fname, false), use_chromakey);
    if (!backdrop[backdrop_no])
    {
      LOG_ERROR("Failed to load backdrop '{}'", fname.c_str());
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

  switch (map.scrolltype)
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
      map.parscroll_x %= backdrop[map.backdrop]->width();
      map.parscroll_y %= backdrop[map.backdrop]->height();
      if (map.parscroll_x < 0)
        map.parscroll_x = map.parscroll_x * 2;
      if (map.parscroll_y < 0)
        map.parscroll_y = map.parscroll_y * 2;
      break;

    case BK_FASTLEFT: // Ironhead
      if (game.mode == GM_NORMAL && !game.frozen && !game.paused)
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
      if (game.curmap == STAGE_KINGS) // intro cutscene
        Renderer::getInstance()->fillScreen(BLACK);
      else
        Renderer::getInstance()->fillScreen(DK_BLUE);
    }
      return;

    default:
      map.parscroll_x = map.parscroll_y = 0;
      LOG_ERROR("map_draw_backdrop: unhandled map scrolling type {}", map.scrolltype);
      break;
  }
  int w = backdrop[map.backdrop]->width();
  int h = backdrop[map.backdrop]->height();

  int mapx = (map.xsize * TILE_W);
  //	int mapy = (map.ysize * TILE_H);
  // hack for ending Maze map
  if (game.curmap == 74)
  {
    map.parscroll_x -= 16;
    mapx += 64;
  }

  if (game.curmap == 31 && Renderer::getInstance()->widescreen)
  {
    //        map.parscroll_y-= 36;
    //        mapy+=64;
  }

  for (y = 0; y < Renderer::getInstance()->screenHeight + map.parscroll_y; y += h)
  {
    for (x = 0; x < Renderer::getInstance()->screenWidth + map.parscroll_x; x += w)
    {
      //		    if ( ((x - map.parscroll_x) < mapx) && ((y - map.parscroll_y) < mapy))
      Renderer::getInstance()->drawSurface(backdrop[map.backdrop], x - map.parscroll_x, y - map.parscroll_y);
    }
  }
}

// blit OSide's BK_FASTLEFT_LAYERS
void DrawFastLeftLayered(void)
{
  int layer_ys[] = {87, 122, 145, 176, Renderer::getInstance()->screenHeight};

  static const int move_spd[] = {0, 1, 2, 4, 8};
  int nlayers                 = 5;
  int y1, y2;
  int i, x;

  if ((game.mode == GM_NORMAL || game.mode == GM_TITLE) && !game.frozen && !game.paused)
    if (--map.parscroll_x <= -480 * 2)
      map.parscroll_x = 0;

  y1 = x = 0;
  // fix for extra height
  if (map.backdrop == 9)
    Renderer::getInstance()->fillScreen(111, 156, 214);
  else if (map.backdrop == 10 && game.curmap != 64)
    Renderer::getInstance()->fillScreen(107, 105, 82);
  else if (map.backdrop == 12)
    Renderer::getInstance()->fillScreen(179, 190, 210);
  else if (map.backdrop == 13)
    Renderer::getInstance()->fillScreen(170, 101, 0);
  else if (map.backdrop == 14)
    Renderer::getInstance()->fillScreen(202, 97, 97);

  for (i = 0; i < nlayers; i++)
  {
    y2 = layer_ys[i];

    if (i) // not the static moon layer?
    {
      x = (map.parscroll_x * move_spd[i]) >> 1;
      //			x %= Renderer::getInstance()->screenWidth;
    }
    Renderer::getInstance()->blitPatternAcross(backdrop[map.backdrop], x, y1, y1, (y2 - y1) + 1);
    y1 = (y2 + 1);
  }
  int mapy = map.displayed_yscroll / CSFI;
  if (mapy < 0 && map.backdrop == 9)
    Renderer::getInstance()->fillRect(0, 0, Renderer::getInstance()->screenWidth, -mapy, 0, 0, 0);
}

void map_flush_graphics()
{
  int i;

  for (i = 0; i < MAX_BACKDROPS; i++)
  {
    delete backdrop[i];
    backdrop[i] = NULL;
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
  Renderer::getInstance()->blitPatternAcross(backdrop[map.backdrop], water_x, water_y, 0, 16);
  water_y += 16;

  Renderer::getInstance()->blitPatternAcross(backdrop[map.backdrop], water_x, water_y, 32, 16);
  water_y += 16;

  // draw the rest of the pattern all the way down
  while (water_y < (Renderer::getInstance()->screenHeight - 1))
  {
    Renderer::getInstance()->blitPatternAcross(backdrop[map.backdrop], water_x, water_y, 16, 32);
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

  blit_y       = -(scroll_y % TILE_H);
  blit_x_start = -(scroll_x % TILE_W);

  // MAP_DRAW_EXTRA_Y etc is 1 if resolution is changed to
  // something not a multiple of TILE_H.
  for (y = 0; y <= (Renderer::getInstance()->screenHeight / TILE_H) + MAP_DRAW_EXTRA_Y; y++)
  {
    blit_x = blit_x_start;

    for (x = 0; x <= (Renderer::getInstance()->screenWidth / TILE_W) + MAP_DRAW_EXTRA_X; x++)
    {
      if (((mapx + x) >= 0) && ((mapy + y) >= 0) && ((mapx + x) < map.xsize) && ((mapy + y) < map.ysize))
      {
        int t = map.tiles[mapx + x][mapy + y];
        // fixes drawing of debug tiles in Stream and Fall maps
        if (((game.curmap == 71) && (tilecode[t] == 0x41)) || ((game.curmap == 31) /*&& (tilecode[t] == 0x46)*/))
        {
        }
        else if ((tileattr[t] & TA_FOREGROUND) == foreground)
        {
          if (tileattr[t] & TA_CURRENT)
          {
            switch (CVTDir(tilecode[t] & 3))
            {
              case LEFT:
                Renderer::getInstance()->sprites.drawSprite(blit_x, blit_y, SPR_WATER_CURRENT, map.motionpos, 0);
                break;
              case RIGHT:
                Renderer::getInstance()->sprites.drawSprite(blit_x, blit_y, SPR_WATER_CURRENT, 7-map.motionpos, 0);
                break;
              case UP:
                Renderer::getInstance()->sprites.drawSprite(blit_x, blit_y, SPR_WATER_CURRENT, map.motionpos, 1);
                break;
              case DOWN:
                Renderer::getInstance()->sprites.drawSprite(blit_x, blit_y, SPR_WATER_CURRENT, 7-map.motionpos, 1);
                break;
              default:
                break;
            }
          }
          else if (tilecode[t] == 0x43)
            Renderer::getInstance()->sprites.drawSprite(blit_x, blit_y, SPR_DESTROYABLE, 0, 0);
          else
            Renderer::getInstance()->tileset.drawTile(blit_x, blit_y, t);
        }
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

  blit_y       = ((-scroll_y) % TILE_H) - TILE_H;
  blit_x_start = ((-scroll_x) % TILE_W) - TILE_W;

  for (y = 0; y <= (Renderer::getInstance()->screenHeight / TILE_H) + 2; y++)
  {
    blit_x = blit_x_start;

    for (x = 0; x <= (Renderer::getInstance()->screenWidth / TILE_W) + 2; x++)
    {
      if (mapx + x <= 0 || mapy + y <= 0 || mapx + x > map.xsize || mapy + y > map.ysize)
      {
        int t = oob_tiles[0];
        if (oob_tile_count == 4)
          t = oob_tiles[abs(mapx + x + 1) % 2 + (abs(mapy + y + 1) % 2) * 2];
        Renderer::getInstance()->tileset.drawTile(blit_x, blit_y, t);
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
#define P_VARY_FROM_CENTER (64 * CSFI)

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
  map.target_x = (player->CenterX() + map.scrollcenter_x) - ((Renderer::getInstance()->screenWidth / 2) * CSFI);

  // Y scrolling
  if (player->lookscroll == UP)
  {
    map.scrollcenter_y -= scroll_adj_rate;
    if (map.scrollcenter_y < -P_VARY_FROM_CENTER)
      map.scrollcenter_y = -P_VARY_FROM_CENTER;
  }
  else if (player->lookscroll == DOWN)
  {
    map.scrollcenter_y += scroll_adj_rate;
    if (map.scrollcenter_y > P_VARY_FROM_CENTER)
      map.scrollcenter_y = P_VARY_FROM_CENTER;
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

  map.target_y = (player->CenterY() + map.scrollcenter_y) - ((Renderer::getInstance()->screenHeight / 2) * CSFI);
}

void map_scroll_do(void)
{
  bool doing_normal_scroll = false;

  if (!map.scroll_locked)
  {
    if (map.focus.has_target)
    { // FON command
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
        if (Renderer::getInstance()->sprites.sprites[t->sprite].frame[t->frame].dir[t->dir].drawpoint.equ(0, 0))
        {
          map.target_x = map.focus.target->CenterX() - ((Renderer::getInstance()->screenWidth / 2) * CSFI);
          map.target_y = map.focus.target->CenterY() - ((Renderer::getInstance()->screenHeight / 2) * CSFI);
        }
        else
        {
          map.target_x = map.focus.target->x - ((Renderer::getInstance()->screenWidth / 2) * CSFI);
          map.target_y = map.focus.target->y - ((Renderer::getInstance()->screenHeight / 2) * CSFI);
        }
      }
    }
    else
    {
      if (!player->hide)
      {
        scroll_normal();

#if defined(DEBUG)
        if (!inputs[DEBUG_MOVE_KEY])
#endif
          doing_normal_scroll = true;
      }
    }
  }

  map.real_xscroll += (double)(map.target_x - map.real_xscroll) / (double)map.scrollspeed;
  map.real_yscroll += (double)(map.target_y - map.real_yscroll) / (double)map.scrollspeed;

  map.displayed_xscroll = (map.real_xscroll + map.phase_adj);
  map.displayed_yscroll = map.real_yscroll; // we don't compensate on Y, because player falls > 2 pixels per frame

  if (doing_normal_scroll)
  {
    run_phase_compensator();
    // dump_phase_data();
  }
  else
  {
    map.phase_adj -= MAP_PHASE_ADJ_SPEED;
    if (map.phase_adj < 0)
      map.phase_adj = 0;
  }

  // custom xscroll on ending "maps"
  switch (game.curmap)
  {
    case 74:
    case 75:
    case 76:
    case 77:
    case 89:
    case 93:
      if (rtl())
      {
        map.displayed_xscroll = ((Renderer::getInstance()->screenWidth / 2) * CSFI);
      }
      else
      {
        map.displayed_xscroll = -(((Renderer::getInstance()->screenWidth - (map.xsize*TILE_W)) / 2) * CSFI);
      }
      map.displayed_yscroll = 0;
      break;
    case 78:
      // carefully crafted magic number
      if (rtl())
      {
        map.displayed_xscroll = ((Renderer::getInstance()->screenWidth - 160) * CSFI);
      }
      else
      {
        map.displayed_xscroll = -((Renderer::getInstance()->screenWidth/2 - 208) * CSFI);
      }
      map.displayed_yscroll = 0;
      break;
    default:
      map_sanitycheck();
      break;
  }

  // do quaketime after sanity check so quake works in
  // small levels like Shack.
  if (game.quaketime)
  {
    if (!map.scroll_locked)
    {
      int pushx, pushy;

      if (game.megaquaketime) // Ballos fight
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
      if (map.real_yscroll < 0)
        map.real_yscroll = 0;
      if (map.real_yscroll > (15 * CSFI))
        map.real_yscroll = (15 * CSFI);

      map.displayed_yscroll += pushy;
      if (map.displayed_yscroll < 0)
        map.displayed_yscroll = 0;
      if (map.displayed_yscroll > (15 * CSFI))
        map.displayed_yscroll = (15 * CSFI);
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
    // debug("%d", phase_offs);

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
  int phase_offs  = abs(map.real_xscroll - player->x) % 512;
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
#define MAP_BORDER_AMT (8 * CSFI)
  if (map.real_xscroll < MAP_BORDER_AMT)
    map.real_xscroll = MAP_BORDER_AMT;
  if (map.real_yscroll < MAP_BORDER_AMT)
    map.real_yscroll = MAP_BORDER_AMT;
  if (map.real_xscroll > map.maxxscroll)
    map.real_xscroll = map.maxxscroll;
  if (map.real_yscroll > map.maxyscroll)
    map.real_yscroll = map.maxyscroll;

  if (map.displayed_xscroll < MAP_BORDER_AMT)
    map.displayed_xscroll = MAP_BORDER_AMT;
  if (map.displayed_yscroll < MAP_BORDER_AMT)
    map.displayed_yscroll = MAP_BORDER_AMT;
  if (map.displayed_xscroll > map.maxxscroll)
    map.displayed_xscroll = map.maxxscroll;
  if (map.displayed_yscroll > map.maxyscroll)
    map.displayed_yscroll = map.maxyscroll;
}

void map_scroll_jump(int x, int y)
{
  map.target_x     = x - ((Renderer::getInstance()->screenWidth / 2) * CSFI);
  map.target_y     = y - ((Renderer::getInstance()->screenHeight / 2) * CSFI);
  map.real_xscroll = map.target_x;
  map.real_yscroll = map.target_y;

  map.displayed_xscroll = map.real_xscroll;
  map.displayed_yscroll = map.real_yscroll;
  map.phase_adj         = 0;

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
  { // why do we do this?
    map.real_xscroll = map.target_x;
    map.real_yscroll = map.target_y;
  }
}

// set the map focus and scroll speed.
// if o is specified, focuses on that object.
// if o is NULL, focuses on the player.
void map_focus(Object *o, int spd)
{
  map.focus.target     = o;
  map.focus.has_target = (o != NULL);

  map.scrollspeed   = spd;
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
  SmokeXY(xa, ya, nclouds, TILE_W / 2, TILE_H / 2, push_behind);

  if (boomflash)
    effect(xa, ya, EFFECT_BOOMFLASH);
}

const std::string &map_get_stage_name(int mapno)
{
  static std::string stagename;
  stagename = (std::string) "stage_" + stages[mapno].filename;
  if (_(stagename) == stagename)
  {
    stagename = stages[mapno].stagename;
    if (mapno == STAGE_KINGS)
      stagename = ""; //"Studio Pixel Presents";
    return _(stagename);
  }
  else
    return _(stagename);
}

// show map name for "ticks" ticks
void map_show_map_name()
{
  if (rtl())
  {
    game.mapname_x       = (Renderer::getInstance()->screenWidth / 2) + (Renderer::getInstance()->font.getWidth(map_get_stage_name(game.curmap)) / 2);
  }
  else
  {
    game.mapname_x       = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->font.getWidth(map_get_stage_name(game.curmap)) / 2);
  }
  game.showmapnametime = 120;
}

void map_draw_map_name(void)
{
  if (game.showmapnametime)
  {
    Renderer::getInstance()->font.draw(game.mapname_x, 84, map_get_stage_name(game.curmap), 0xFFFFFF, true);
    game.showmapnametime--;
  }
}

// animate all motion tiles
void AnimateMotionTiles(void)
{
  map.motionpos ++;
  if (map.motionpos >= TILE_W / 2)
    map.motionpos = 0;
}

// attempts to find an object with id2 matching the given value else returns NULL
Object *FindObjectByID2(int id2)
{
  Object *result = ID2Lookup[id2];

  if (result)
  {
    LOG_DEBUG("FindObjectByID2: ID2 {:#04d} found: type {}; coords: ({}, {})", id2, DescribeObjectType(ID2Lookup[id2]->type),
         ID2Lookup[id2]->x / CSFI, ID2Lookup[id2]->y / CSFI);
  }
  else
  {
    LOG_WARN("FindObjectByID2: no such object {:#04d}", id2);
  }

  return result;
}
