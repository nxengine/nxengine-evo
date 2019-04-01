
#include "pause.h"

#include "../autogen/sprites.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../nx.h"
#include "../screeneffect.h"
#include "../endgame/credits.h"
#include "../inventory.h"
#include "../map_system.h"
using namespace NXE::Graphics;
#include "dialog.h"
using namespace Options;

Dialog *dlg;
unsigned int mm_cursel;

void _resume(ODItem *item, int dir)
{
  lastinputs[F1KEY] = true;
  game.pause(false);
  delete dlg;
  dlg = nullptr;
}

void _options(ODItem *item, int dir)
{
  game.pause(GP_OPTIONS);
}

void _reset(ODItem *item, int dir)
{
  lastinputs[F2KEY] = true;
  game.reset();
  delete dlg;
  dlg = nullptr;
}

void _exit(ODItem *item, int dir)
{
  lastinputs[ESCKEY] = true;
  game.running       = false;
}

bool pause_init(int param)
{
  memset(lastinputs, 1, sizeof(lastinputs));
  dlg = new Dialog();
  dlg->AddItem("Resume", _resume);
  dlg->AddItem("Options", _options);
  dlg->AddItem("Reset", _reset);
  dlg->AddItem("Quit", _exit);
  dlg->SetSelection(mm_cursel);
  dlg->ShowFull();
  int maxsize = 0;
  for (auto &item : dlg->Items())
  {
    int x = Renderer::getInstance()->font.getWidth(_(item->text));
    if (x > maxsize)
      maxsize = x;
  }
  dlg->SetSize(maxsize + 60, 70);
  return 0;
}

void pause_tick()
{
  if (dlg == nullptr) return;
  DrawScene();
  if (game.mode == GM_NORMAL)
    DrawStatusBar();
  if (game.mode == GM_INVENTORY)
    DrawInventory();
  if (game.mode == GM_MAP_SYSTEM)
    ms_draw();

  fade.Draw();

  textbox.Draw();

  if (game.mode == GM_CREDITS)
  {
    credit_draw();
  }

  Renderer::getInstance()->tintScreen();

  dlg->RunInput();
  if (dlg == nullptr) return;
  dlg->Draw();

  if (justpushed(ESCKEY))
  {
    _resume(NULL, 0);
    return;
  }
}
