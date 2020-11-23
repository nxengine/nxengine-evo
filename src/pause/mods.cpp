
#include "pause.h"

#include "../autogen/sprites.h"
#include "../game.h"
#include "../map.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../nx.h"
#include "../ResourceManager.h"

using namespace NXE::Graphics;

#include "dialog.h"
using namespace Options;

Dialog *moddlg;
std::vector<std::string> _mods;
extern std::vector<void *> optionstack;

void _mod_run(ODItem *item, int dir)
{
  ResourceManager::getInstance()->setMod(_mods.at(item->id));
  game.tsc->Close();
  load_stages();
  game.tsc->Init();
  game.reset();
}

void _mod_reset(ODItem *item, int dir)
{
  ResourceManager::getInstance()->setMod("");
  game.tsc->Close();
  load_stages();
  game.tsc->Init();
  game.reset();
}

void _mod_return(ODItem *item, int dir)
{
  game.pause(false);
}

bool mods_init(int param)
{
  memset(lastinputs, 1, sizeof(lastinputs));
  moddlg = new Dialog();
  moddlg->AddItem("Original game", _mod_reset);
  int i = 0;
  for (auto &kv: ResourceManager::getInstance()->mods())
  {
    moddlg->AddItem(kv.second.name.c_str(), _mod_run, NULL, i);
    _mods.push_back(kv.second.dir);
    i++;
  }

  moddlg->AddSeparator();
  moddlg->AddItem("Return", _mod_return);
  moddlg->SetSelection(0);
  moddlg->ShowFull();
  int maxsize = 0;
  for (auto &item : moddlg->Items())
  {
    int x = Renderer::getInstance()->font.getWidth(_(item->text));
    if (x > maxsize)
      maxsize = x;
  }
//  moddlg->SetSize(maxsize + 60, 70);
  return 0;
}

void mods_tick()
{
   if (moddlg != NULL) {
       Renderer::getInstance()->clearScreen(BLACK);
       moddlg->Draw();
       moddlg->RunInput();
   }
}

void mods_close()
{
  optionstack.clear();
  delete moddlg;
  moddlg = NULL;
}