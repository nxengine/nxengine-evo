
// the Map System
#include "map_system.h"

#include "autogen/sprites.h"
#include "graphics/Renderer.h"
#include "nx.h"
using namespace NXE::Graphics;
#include "game.h"
#include "map.h"
#include "player.h"

#define MS_EXPANDING 0
#define MS_DISPLAYED 1
#define MS_CONTRACTING 2

// # of frames in the expand/contract animation
#define EXPAND_LENGTH 8

#define BANNER_TOP 7
#define BANNER_BTM 23

//#define CSF 9

static struct
{
  int x, y; // the position of the upper-left corner of the image
  int w, h; // size of image

  int expandframe; // for expand/contract effect
  int current_row; // scan down effect

  int px, py; // the position of the you-are-here dot
  int timer;  // for the flashing

  int state;     // expanding, displayed, contracting
  int return_gm; // game mode to return to

  const char *bannertext;
  int textx, texty;

} ms;

// expand/contract effect
static void draw_expand(void)
{
  int x1, y1, x2, y2;

  int wd = (map.xsize * ms.expandframe) / EXPAND_LENGTH;
  int ht = (map.ysize * ms.expandframe) / EXPAND_LENGTH;

  x1 = (Renderer::getInstance()->screenWidth / 2) - (wd / 2);
  y1 = (Renderer::getInstance()->screenHeight / 2) - (ht / 2);
  x2 = (Renderer::getInstance()->screenWidth / 2) + (wd / 2);
  y2 = (Renderer::getInstance()->screenHeight / 2) + (ht / 2);

  Renderer::getInstance()->fillRect(x1, y1, x2, y2, DK_BLUE);
}

static void draw_banner(void)
{
  Renderer::getInstance()->fillRect(0, BANNER_TOP, Renderer::getInstance()->screenWidth, BANNER_BTM, NXColor(0, 0, 0));
  Renderer::getInstance()->font.draw(ms.textx, ms.texty, _(ms.bannertext));
}

/*
void c------------------------------() {}
*/

static int get_color(int tilecode)
{
  switch (tilecode)
  {
    case 0:
      return 0;

    case 0x01:
    case 0x02:
    case 0x40:
    case 0x44:
    case 0x51:
    case 0x52:
    case 0x55:
    case 0x56:
    case 0x60:
    case 0x71:
    case 0x72:
    case 0x75:
    case 0x76:
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
      return 1;

    case 0x43:
    case 0x50:
    case 0x53:
    case 0x54:
    case 0x57:
    case 0x63:
    case 0x70:
    case 0x73:
    case 0x74:
    case 0x77:
      return 2;

    default:
      return 3;
  }
}

bool ms_init(int return_to_mode)
{
  memset(&ms, 0, sizeof(ms));
  ms.return_gm      = return_to_mode;
  ms.w              = map.xsize;
  ms.h              = map.ysize;

  ms.x = (Renderer::getInstance()->screenWidth / 2) - (ms.w / 2);
  ms.y = (Renderer::getInstance()->screenHeight / 2) - (ms.h / 2);

  // where will we put the dot?
  ms.px = ms.x + ((player->x / CSFI) / TILE_W);
  ms.py = ms.y + ((player->y / CSFI) / TILE_H);

  ms.bannertext = stages[game.curmap].stagename;
  ms.textx      = (Renderer::getInstance()->screenWidth / 2) + (rtl() ? (Renderer::getInstance()->font.getWidth(ms.bannertext) / 2) : -(Renderer::getInstance()->font.getWidth(ms.bannertext) / 2));
  ms.texty      = BANNER_TOP + 3;

  return 0;
}

void ms_close(void)
{
  memset(inputs, 0, sizeof(inputs));
}

void ms_tick(void)
{
  DrawScene();

  if (ms.state == MS_EXPANDING)
  {
    ms.expandframe++;

    if (ms.expandframe > EXPAND_LENGTH)
      ms.state = MS_DISPLAYED;
  }

  if (ms.state == MS_DISPLAYED)
  {
    if (ms.current_row < map.ysize)
      ms.current_row++;
    if (ms.current_row < map.ysize)
      ms.current_row++;

    // you-are-here dot
    ms.timer++;

    // dismissal
    if (justpushed(DECLINE_BUTTON) || justpushed(ACCEPT_BUTTON) || justpushed(MAPSYSTEMKEY))
    {
      ms.state = MS_CONTRACTING;
    }
  }
  else if (ms.state == MS_CONTRACTING)
  {
    ms.expandframe--;

    if (ms.expandframe <= 0)
    {
      int param = (ms.return_gm == GM_INVENTORY) ? 1 : 0;
      game.setmode(ms.return_gm, param);
    }
  }
  ms_draw();
}

void ms_draw(void)
{
  draw_banner();

  if (ms.state == MS_EXPANDING)
  {
    draw_expand();
  }

  if (ms.state == MS_DISPLAYED)
  {
    // draw map
    Renderer::getInstance()->drawRect(ms.x - 1, ms.y - 1, ms.x + ms.w, ms.y + ms.h, DK_BLUE);
    Renderer::getInstance()->fillRect(ms.x - 1, ms.y - 1, ms.x + ms.w, ms.y + ms.h, DK_BLUE);
    for (int y = 0; y < ms.current_row; y++)
    {
      for (int x = 0; x < map.xsize; x++)
      {
        int tc = tilecode[map.tiles[x][y]];

        Renderer::getInstance()->sprites.drawSprite(ms.x + x, ms.y + y, SPR_MAP_PIXELS, get_color(tc));
      }
    }

    // you-are-here dot
    if (ms.timer & 8)
      Renderer::getInstance()->sprites.drawSprite(ms.px, ms.py, SPR_MAP_PIXELS, 4);
  }
  else if (ms.state == MS_CONTRACTING)
  {
    draw_expand();
  }
}
