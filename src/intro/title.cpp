
#include "title.h"

#include "../TextBox/TextBox.h"
#include "../autogen/sprites.h"
#include "../Utils/Logger.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../map.h"
#include "../niku.h"
#include "../nx.h"
#include "../profile.h"
#include "../ResourceManager.h"
#include "../settings.h"
#include "../sound/SoundManager.h"
#include "../statusbar.h"
using namespace NXE::Graphics;

// music and character selections for the different Counter times
static struct
{
  uint32_t timetobeat;
  int sprite;
  uint8_t frames[4];
  int songtrack;
  int backdrop;
} titlescreens[] = {
    {(3 * 3000), SPR_SUE, {2,3,4,5}, 2, 14},     // 3 mins	- Sue & Safety
    {(4 * 3000), SPR_KING, {4,5,6,7}, 41, 13},   // 4 mins	- King & White
    {(5 * 3000), SPR_TOROKO_SHACK, {1,2,3,2}, 40, 12}, // 5 mins	- Toroko & Toroko's Theme
    {(6 * 3000), SPR_CURLY, {0,1,2,3}, 36, 10},  // 6 mins	- Curly & Running Hell
    {0xFFFFFFFF, SPR_MYCHAR, {0,1,0,2}, 24, 9}   // default
};

// artifical fake "loading" delay between selecting an option and it being executed,
// because it actually doesn't look good if we respond instantly.
#define SELECT_DELAY 30
#define SELECT_LOAD_DELAY 20 // delay when leaving the multisave Load dialog
#define SELECT_MENU_DELAY 8  // delay from Load to load menu

static struct
{
  int sprite;
  uint8_t* frames;
  int cursel;
  int selframe, seltimer;
  int selchoice, seldelay;
  int kc_pos;
  bool in_multiload;

  uint32_t besttime; // Nikumaru display
} title;

typedef struct
{
  std::string text;
  bool enabled;
} menuitem;

std::vector<menuitem> _menuitems;

static void draw_title()
{
  // background is dk grey, not pure black
  Renderer::getInstance()->fillScreen(0x20, 0x20, 0x20);
  map_draw_backdrop();
  //	DrawFastLeftLayered();

  // top logo
  int tx = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->sprites.sprites[SPR_TITLE].w / 2) - 2;
  Renderer::getInstance()->sprites.drawSprite(tx, 40, SPR_TITLE);

  // draw menu

  int cx = (Renderer::getInstance()->screenWidth / 2) + (rtl() ? 32 : -32);
  int cy = (Renderer::getInstance()->screenHeight / 2) - 8;

  TextBox::DrawFrame((Renderer::getInstance()->screenWidth / 2) - 64, cy - 16 , 128, 96);

  for (size_t i = 0; i < _menuitems.size(); i++)
  {
    if (_menuitems[i].enabled)
    {
      if (rtl())
      {
        Renderer::getInstance()->font.draw(cx - 10, cy, _(_menuitems[i].text));
      }
      else
      {
        Renderer::getInstance()->font.draw(cx + 10, cy, _(_menuitems[i].text));
      }
    }
    else
    {
      if (rtl())
      {
        Renderer::getInstance()->font.draw(cx - 10, cy, _(_menuitems[i].text), 0x666666);
      }
      else
      {
        Renderer::getInstance()->font.draw(cx + 10, cy, _(_menuitems[i].text), 0x666666);
      }
    }

    if (i == (size_t)title.cursel)
    {
      if (rtl())
      {
        Renderer::getInstance()->sprites.drawSprite(cx, cy - 1, title.sprite, title.frames[title.selframe], LEFT);
      }
      else
      {
        Renderer::getInstance()->sprites.drawSprite(cx - 16, cy - 1, title.sprite, title.frames[title.selframe]);
      }
    }

    cy += 12;
  }

  // animate character
  if (++title.seltimer > 8)
  {
    title.seltimer = 0;
    if (++title.selframe >= 4)
      title.selframe = 0;
  }

  // accreditation
  cx        = (Renderer::getInstance()->screenWidth / 2) - (Renderer::getInstance()->sprites.sprites[SPR_PIXEL_FOREVER].w / 2);
  int acc_y = Renderer::getInstance()->screenHeight - 48;
  Renderer::getInstance()->sprites.drawSprite(cx, acc_y, SPR_PIXEL_FOREVER);

  // version
  int wd = Renderer::getInstance()->font.getWidth(NXVERSION);
  cx     = (Renderer::getInstance()->screenWidth / 2) + (rtl() ? (wd / 2) : -(wd / 2));
  Renderer::getInstance()->font.draw(cx, acc_y + Renderer::getInstance()->sprites.sprites[SPR_PIXEL_FOREVER].h + 4, NXVERSION, 0xf3e298);

  // draw Nikumaru display
  if (title.besttime != 0xffffffff)
    niku_draw(title.besttime, true);
}

static int kc_table[] = {UPKEY, UPKEY, DOWNKEY, DOWNKEY, LEFTKEY, RIGHTKEY, LEFTKEY, RIGHTKEY, -1};

void run_konami_code()
{
  if (justpushed(UPKEY) || justpushed(DOWNKEY) || justpushed(LEFTKEY) || justpushed(RIGHTKEY))
  {
    if (justpushed(kc_table[title.kc_pos]))
    {
      title.kc_pos++;
      if (kc_table[title.kc_pos] == -1)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
        game.debug.god = 1;
        title.kc_pos = 0;
      }
    }
    else
    {
      title.kc_pos = 0;
    }
  }
}

static void handle_input()
{
  if (justpushed(DOWNKEY))
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
    do
    {
      if (++title.cursel >= (int)_menuitems.size())
        title.cursel = 0;
    } while (!_menuitems.at(title.cursel).enabled);
  }
  else if (justpushed(UPKEY))
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
    do
    {
      if (--title.cursel < 0)
        title.cursel = _menuitems.size()-1;
    } while (!_menuitems.at(title.cursel).enabled);
  }

  if (justpushed(ACCEPT_BUTTON) || justpushed(ENTERKEY))
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
    int choice = title.cursel;

    // handle case where user selects Load but there is no savefile,
    // or the last_save_file is deleted.
    if (title.cursel == 1)
    {
      if (!ProfileExists(settings->last_save_slot))
      {
        bool foundslot = false;
        for (int i = 0; i < MAX_SAVE_SLOTS; i++)
        {
          if (ProfileExists(i))
          {
            LOG_WARN("Last save file {} missing. Defaulting to {} instead.", settings->last_save_slot, i);
            settings->last_save_slot = i;
            foundslot                = true;
          }
        }

        // there are no save files. Start a new game instead.
        if (!foundslot)
        {
          LOG_WARN("No save files found. Starting new game instead.");
          choice = 0;
        }
      }
    }

    if (choice == 1)// && settings->multisave)
    {
      title.selchoice = 10;
      title.seldelay  = SELECT_MENU_DELAY;
    }
    else
    {
      title.selchoice = choice;
      if (choice == 0)
        title.seldelay = SELECT_DELAY;
      else
        title.seldelay = 1;
      //			music(0);
    }
  }

  run_konami_code();
}

static void selectoption(int index)
{
  switch (index)
  {
    case 0: // New
    {
      NXE::Sound::SoundManager::getInstance()->music(0);

      game.switchstage.mapno = NEW_GAME_FROM_MENU;
      game.setmode(GM_NORMAL);
    }
    break;

    case 1: // Load
    {
      NXE::Sound::SoundManager::getInstance()->music(0);

      game.switchstage.mapno = LOAD_GAME_FROM_MENU;
      game.setmode(GM_NORMAL);
    }
    break;

    case 2: // Options
    {
      //			music(0);
      game.pause(GP_OPTIONS);
    }
    break;
    case 3: // Mods
    {
      game.pause(GP_MODS);
    }
    break;
    case 4: // Quit
    {
      NXE::Sound::SoundManager::getInstance()->music(0);
      game.running = false;
    }
    break;

    case 10: // Load Menu (multisave)
    {
      textbox.SetVisible(true);
      textbox.SaveSelect.SetVisible(true, SS_LOADING);
      title.in_multiload = true;
    }
    break;
  }
}

bool title_init(int param)
{
  memset(&title, 0, sizeof(title));
  //	game.switchstage.mapno = 0;
  game.switchstage.mapno        = TITLE_SCREEN;
  game.switchstage.eventonentry = 0;
  game.showmapnametime          = 0;
  textbox.SetVisible(false);

  title.besttime = niku_load();

  // select a title screen based on Nikumaru time
  int t;
  for (t = 0;; t++)
  {
    if (title.besttime < titlescreens[t].timetobeat || titlescreens[t].timetobeat == 0xffffffff)
    {
      break;
    }
  }

  title.sprite = titlescreens[t].sprite;
  title.frames = titlescreens[t].frames;
  NXE::Sound::SoundManager::getInstance()->music(titlescreens[t].songtrack);
  map_set_backdrop(titlescreens[t].backdrop);
  map.scrolltype = BK_FASTLEFT_LAYERS;
  map.motionpos  = 0;

  if (AnyProfileExists())
    title.cursel = 1; // Load Game
  else
    title.cursel = 0; // New Game

  _menuitems.clear();
  _menuitems.push_back({"New game",true});

  if (AnyProfileExists())
    _menuitems.push_back({"Load game",true});
  else
    _menuitems.push_back({"Load game",false});

  _menuitems.push_back({"Options",true});

  if (ResourceManager::getInstance()->mods().size() > 0 )
    _menuitems.push_back({"Mods",true});
  else
    _menuitems.push_back({"Mods",false});

  _menuitems.push_back({"Quit",true});

  return 0;
}

void title_tick()
{
  if (!title.in_multiload)
  {
    if (title.seldelay > 0)
    {
      Renderer::getInstance()->fillScreen(BLACK);

      title.seldelay--;
      if (!title.seldelay)
        selectoption(title.selchoice);

      return;
    }

    handle_input();
    draw_title();
  }
  else
  {
    Renderer::getInstance()->fillScreen(BLACK);

    if (!textbox.SaveSelect.IsVisible())
    { // selection was made, and settings.last_save_slot is now set appropriately

      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);

      textbox.SetVisible(false);
      title.in_multiload = false;
      if (!textbox.SaveSelect.Aborted())
      {
        title.selchoice = 1;
        title.seldelay  = SELECT_LOAD_DELAY;
      }
    }
    else
    {
      textbox.Tick();
    }
  }
}
