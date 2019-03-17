
// "XX1" island-crashes cutscene from "good ending"
// and island-looks-like-it-gonna-crash but it's ok from secret ending
#include "island.h"

#include "../autogen/sprites.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../nx.h"
#include "../player.h"
#include "../statusbar.h"
using namespace NXE::Graphics;

static struct
{
  int x, y;
  int timer, scene_length;
  int speed;
  bool survives;

  int scene_x, scene_y;
  int trees_x, trees_y;
} island;

bool island_init(int parameter)
{
  memset(&island, 0, sizeof(island));

  island.timer        = 0;
  island.speed        = 0x33;
  island.survives     = parameter;
  island.scene_length = (!island.survives) ? 900 : 750;

  island.scene_x = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->sprites.sprites[SPR_ISLAND_SCENE].w / 2);
  island.scene_y = (Renderer::getInstance()->screenHeight / 2) - (Renderer::getInstance()->sprites.sprites[SPR_ISLAND_SCENE].h / 2);

  island.trees_x = island.scene_x;
  island.trees_y = (island.scene_y + Renderer::getInstance()->sprites.sprites[SPR_ISLAND_SCENE].h) - Renderer::getInstance()->sprites.sprites[SPR_ISLAND_TREES].h;

  island.x = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->sprites.sprites[SPR_ISLAND].w / 2);
  island.y = (island.scene_y - Renderer::getInstance()->sprites.sprites[SPR_ISLAND].h) * CSFI;

  return 0;
}

/*
void c------------------------------() {}
*/

void island_tick()
{
  if (island.timer >= island.scene_length)
  {
    game.setmode(GM_NORMAL);
    return;
  }

  if (island.survives)
  {
    switch (island.timer)
    {
      case 350:
        island.speed = 0x19;
        break;
      case 500:
        island.speed = 0x0C;
        break;
      case 600:
        island.speed = 0;
        break;
    }
  }

  island.y += island.speed;
  island.timer++;

  // draw the scene
  Renderer::getInstance()->clearScreen(BLACK);

  Renderer::getInstance()->setClip(island.scene_x, island.scene_y, Renderer::getInstance()->sprites.sprites[SPR_ISLAND_SCENE].w, Renderer::getInstance()->sprites.sprites[SPR_ISLAND_SCENE].h);

  Renderer::getInstance()->sprites.drawSprite(island.scene_x, island.scene_y, SPR_ISLAND_SCENE);
  Renderer::getInstance()->sprites.drawSprite(island.x, (island.y / CSFI), SPR_ISLAND);

  Renderer::getInstance()->sprites.drawSprite(island.trees_x, island.trees_y, SPR_ISLAND_TREES);

  Renderer::getInstance()->clearClip();

  if (player->equipmask & EQUIP_NIKUMARU)
    niku_draw(game.counter);
}

/*
void c------------------------------() {}
*/
