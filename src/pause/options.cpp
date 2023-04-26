#include "options.h"

#include "../nx.h"
#include "dialog.h"
#include "message.h"
#include "../ResourceManager.h"
#include "../common/misc.h"
#include "../game.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../map.h"
#include "../settings.h"
#include "../sound/SoundManager.h"
using namespace Options;
using namespace NXE::Graphics;

std::vector<void *> optionstack;

void DialogDismissed();
static void EnterMainMenu();
static void EnterSoundMenu(ODItem *item, int dir);
static void EnterGraphicsMenu(ODItem *item, int dir);
static void EnterControlsMenu(ODItem *item, int dir);
static void EnterRebindMenu(ODItem *item, int dir);

void LeavingMainMenu();
void _res_get(ODItem *item);
void _res_change(ODItem *item, int dir);
void _widescreen_get(ODItem *item);
void _widescreen_change(ODItem *item, int dir);
void _facepics_get(ODItem *item);
void _facepics_change(ODItem *item, int dir);
void _lights_get(ODItem *item);
void _lights_change(ODItem *item, int dir);

void _lang_get(ODItem *item);
void _lang_change(ODItem *item, int dir);

void _sound_change(ODItem *item, int dir);
void _sound_get(ODItem *item);
void _music_change(ODItem *item, int dir);
void _music_get(ODItem *item);
void _tracks_change(ODItem *item, int dir);
void _tracks_get(ODItem *item);
void _sfx_volume_change(ODItem *item, int dir);
void _sfx_volume_get(ODItem *item);
void _music_volume_change(ODItem *item, int dir);
void _music_volume_get(ODItem *item);
void _music_interpolation_change(ODItem *item, int dir);
void _music_interpolation_get(ODItem *item);
void _rumble_change(ODItem *item, int dir);
void _rumble_get(ODItem *item);
void _strafe_change(ODItem *item, int dir);
void _strafe_get(ODItem *item);
void _scheme_change(ODItem *item, int dir);
void _scheme_get(ODItem *item);
void _scheme_get2(ODItem *item);
static void _upd_control(ODItem *item);
static void _edit_control(ODItem *item, int dir);
static void _finish_control_edit(Message *msg);

#define SLIDE_SPEED 32
#define SLIDE_DIST (3 * SLIDE_SPEED)

static struct
{
  Dialog *dlg, *subdlg;
  Dialog *dismiss_on_focus;
  unsigned int mm_cursel;
  bool InMainMenu;
  bool InBindingMenu;
  int xoffset;

  int32_t remapping_key;
  in_action new_sdl_key;
} opt;

bool options_init(int retmode)
{
  memset(&opt, 0, sizeof(opt));
  Options::init_objects();
  opt.dlg = new Dialog;

  opt.xoffset = SLIDE_DIST;
  opt.dlg->offset(-SLIDE_DIST, 0);

  EnterMainMenu();
  opt.dlg->ondismiss = DialogDismissed;
  opt.dlg->ShowFull();

  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  return 0;
}

void options_close()
{
  Options::close_objects();
  for (unsigned int i = 0; i < optionstack.size(); i++)
  {
    FocusHolder *fh = (FocusHolder *)optionstack.at(i);
    delete fh;
  }
  optionstack.clear();
  settings_save();
}

/*
void c------------------------------() {}
*/

void options_tick()
{
  unsigned int i;
  FocusHolder *fh;

  Renderer::getInstance()->clearScreen(BLACK);
  Options::run_and_draw_objects();

  fh = (FocusHolder *)optionstack.at(optionstack.size() - 1);
  if (fh)
  {
    fh->RunInput();
    if (game.paused != GP_OPTIONS)
      return;

    fh = (FocusHolder *)optionstack.at(optionstack.size() - 1);
    if (fh == opt.dismiss_on_focus && fh)
    {
      opt.dismiss_on_focus = NULL;
      delete fh;
      //			optionstack.erase(optionstack.begin()+(optionstack.size() - 1));
    }
  }

  for (i = 0; i < optionstack.size(); i++)
  {
    fh = (FocusHolder *)optionstack.at(i);
    if (fh)
      fh->Draw();
  }

  if (opt.xoffset > 0)
  {
    opt.dlg->offset(SLIDE_SPEED, 0);
    opt.xoffset -= SLIDE_SPEED;
  }
}

/*
void c------------------------------() {}
*/

void DialogDismissed()
{
  if (opt.InMainMenu)
  {
    memset(inputs, 0, sizeof(inputs));
    game.pause(false);
  }
  else if (opt.InBindingMenu)
  {
    EnterControlsMenu(NULL,0);
  }
  else
  {
    EnterMainMenu();
  }
}

/*
void c------------------------------() {}
*/

static void EnterMainMenu()
{
  Dialog *dlg = opt.dlg;

  dlg->Clear();

  dlg->AddItem("Graphics", EnterGraphicsMenu);
  dlg->AddItem("Sound", EnterSoundMenu);
  dlg->AddItem("Controls", EnterControlsMenu);

  dlg->AddSeparator();

  if (game.mode == GM_TITLE)
    dlg->AddItem("Language: ", _lang_change, _lang_get, -1, OD_CHOICE);

  dlg->AddSeparator();
  dlg->AddDismissalItem();

  dlg->SetSelection(opt.mm_cursel);
  dlg->onclear   = LeavingMainMenu;
  opt.InMainMenu = true;
}

void LeavingMainMenu()
{
  opt.mm_cursel    = opt.dlg->GetSelection();
  opt.dlg->onclear = NULL;
  opt.InMainMenu   = false;
}

static void EnterControlsMenu(ODItem *item, int dir)
{
  Dialog *dlg = opt.dlg;

  dlg->Clear();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  dlg->AddItem("Force feedback: ", _rumble_change, _rumble_get, -1, OD_CHOICE);
  dlg->AddItem("Strafing: ", _strafe_change, _strafe_get, -1, OD_CHOICE);
  dlg->AddSeparator();
  dlg->AddItem("Ok: ", _scheme_change, _scheme_get, -1, OD_CHOICE);
  dlg->AddItem("Cancel: ", _scheme_change, _scheme_get2, -1, OD_CHOICE);
  dlg->AddSeparator();
  dlg->AddItem("Bind keys", EnterRebindMenu);

  dlg->AddSeparator();
  dlg->AddDismissalItem();
  opt.InBindingMenu = false;
}

static void EnterRebindMenu(ODItem *item, int dir)
{
  Dialog *dlg = opt.dlg;
  opt.InBindingMenu = true;
  dlg->Clear();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  dlg->AddItem("Left", _edit_control, _upd_control, LEFTKEY);
  dlg->AddItem("Right", _edit_control, _upd_control, RIGHTKEY);
  dlg->AddItem("Up", _edit_control, _upd_control, UPKEY);
  dlg->AddItem("Down", _edit_control, _upd_control, DOWNKEY);
  dlg->AddItem("Jump", _edit_control, _upd_control, JUMPKEY);
  dlg->AddItem("Strafe", _edit_control, _upd_control, STRAFEKEY);
  dlg->AddItem("Fire", _edit_control, _upd_control, FIREKEY);
  dlg->AddItem("Wpn Prev", _edit_control, _upd_control, PREVWPNKEY);
  dlg->AddItem("Wpn Next", _edit_control, _upd_control, NEXTWPNKEY);
  dlg->AddItem("Inventory", _edit_control, _upd_control, INVENTORYKEY);
  dlg->AddItem("Map", _edit_control, _upd_control, MAPSYSTEMKEY);
  dlg->AddItem("Pause", _edit_control, _upd_control, ESCKEY);

  dlg->AddSeparator();
  dlg->AddDismissalItem();
  opt.InBindingMenu = true;
}

static void EnterGraphicsMenu(ODItem *item, int dir)
{
  Dialog *dlg = opt.dlg;

  dlg->Clear();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  dlg->AddItem("Resolution: ", _res_change, _res_get, -1, OD_CHOICE);
  dlg->AddItem("Widescreen: ", _widescreen_change, _widescreen_get, -1, OD_CHOICE);
  dlg->AddItem("Animated facepics: ", _facepics_change, _facepics_get, -1, OD_CHOICE);
  dlg->AddItem("Lights: ", _lights_change, _lights_get, -1, OD_CHOICE);
  dlg->AddSeparator();
  dlg->AddDismissalItem();
}

static void EnterSoundMenu(ODItem *item, int dir)
{
  Dialog *dlg = opt.dlg;

  dlg->Clear();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
  dlg->AddItem("Music: ", _music_change, _music_get, -1, OD_CHOICE);
  dlg->AddItem("Tracks: ", _tracks_change, _tracks_get, -1, OD_CHOICE);
  dlg->AddItem("Sound: ", _sound_change, _sound_get, -1, OD_CHOICE);
  dlg->AddSeparator();
  dlg->AddItem("SFX volume: ", _sfx_volume_change, _sfx_volume_get, -1, OD_CHOICE);
  dlg->AddItem("Music volume: ", _music_volume_change, _music_volume_get, -1, OD_CHOICE);
  dlg->AddItem("Music interpolation: ", _music_interpolation_change, _music_interpolation_get, -1, OD_CHOICE);
  dlg->AddSeparator();
  dlg->AddDismissalItem();
}


void _res_get(ODItem *item)
{
  if (settings->resolution < 0 || settings->resolution >= Renderer::getInstance()->getResolutionCount())
  {
    item->suffix[0] = 0;
  }
  else
  {
    if (settings->resolution == 0) {
      strcpy(item->suffix, "fullscreen");
    } else {
      strcpy(item->suffix, (std::to_string(settings->resolution) + "x").c_str());
    }
  }
}

void _res_change(ODItem *item, int dir)
{
  int numres = Renderer::getInstance()->getResolutionCount();
  int oldres = settings->resolution;
  int newres;

  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DOOR);

  newres = (oldres + dir);
  if (newres >= numres)
    newres = 0;
  if (newres < 0)
    newres = (numres - 1);

  if (!Renderer::getInstance()->setResolution(newres == 0 ? 1 : newres, settings->widescreen)) {
    new Message("Resolution change failed");
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
    return;
  }

  settings->resolution = newres;

  if (newres == 0) {
    if (!Renderer::getInstance()->setFullscreen(true)) {
      new Message("Fullscreen mode change failed");
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
      return;
    }
  } else if (oldres == 0) {
    if (!Renderer::getInstance()->setFullscreen(false)) {
      new Message("Fullscreen mode change failed");
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
    }
  }
}

void _lang_get(ODItem *item)
{
  std::vector<std::string> langs = ResourceManager::getInstance()->languages();
  for (auto &l : langs)
  {
    if (strcmp(settings->language, l.c_str()) == 0)
    {
      strcpy(item->suffix, l.c_str());
    }
  }
}

void _lang_change(ODItem *item, int dir)
{
  std::vector<std::string> langs = ResourceManager::getInstance()->languages();
  int i                          = 0;
  for (auto &l : langs)
  {
    if (l == settings->language)
      break;
    i++;
  }
  i += dir;
  if (i < 0)
  {
    i = langs.size() - 1;
  }
  if (i >= (int)langs.size())
  {
    i = 0;
  }
  memset(settings->language, 0, 256);
  strncpy(settings->language, langs[i].c_str(), 255);
  game.lang->load();
//  font_reload();
  game.tsc->Init();
  if (!Renderer::getInstance()->flushAll())
  {
    LOG_CRITICAL("Failed to re-init graphics");
    exit(-1);
  }
}

void _widescreen_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->widescreen]);
}

void _widescreen_change(ODItem *item, int dir)
{
  settings->widescreen ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
  if (!Renderer::getInstance()->setResolution(settings->resolution, settings->widescreen)) {
    new Message("Resolution change failed");
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
  }
}

void _facepics_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->animated_facepics]);
}

void _facepics_change(ODItem *item, int dir)
{
  settings->animated_facepics ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _lights_change(ODItem *item, int dir)
{
  settings->lights ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _lights_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->lights]);
}

void _sound_change(ODItem *item, int dir)
{
  settings->sound_enabled ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _sound_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->sound_enabled]);
}

void _strafe_change(ODItem *item, int dir)
{
  settings->strafing ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _strafe_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->strafing]);
}

void _music_change(ODItem *item, int dir)
{
  int result = settings->music_enabled + dir;
  if (result < 0)
    result = 2;
  if (result > 2)
    result = 0;
  NXE::Sound::SoundManager::getInstance()->enableMusic(result);
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _music_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On", "Boss Only"};
  strcpy(item->suffix, strs[settings->music_enabled]);
}

void _tracks_change(ODItem *item, int dir)
{
  std::vector<std::string> names = NXE::Sound::SoundManager::getInstance()->music_dir_names();
  int result = settings->new_music + dir;

  if (result < 0)
    result = names.size()-1;
  if (result >= (int)names.size())
    result = 0;
  NXE::Sound::SoundManager::getInstance()->setNewmusic(result);
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _tracks_get(ODItem *item)
{
  std::vector<std::string> names = NXE::Sound::SoundManager::getInstance()->music_dir_names();
  strcpy(item->suffix, names.at(settings->new_music).c_str());
}

void _sfx_volume_change(ODItem *item, int dir)
{
  settings->sfx_volume += 5*dir;

  if (settings->sfx_volume <= 0)
    settings->sfx_volume = 0;
  else if (settings->sfx_volume >= 100)
    settings->sfx_volume = 100;
  NXE::Sound::SoundManager::getInstance()->updateSfxVolume();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _sfx_volume_get(ODItem *item)
{
  char str[10];
  sprintf(str, "%d%%", settings->sfx_volume);
  strcpy(item->suffix, str);
}

void _music_volume_change(ODItem *item, int dir)
{
  settings->music_volume += 5*dir;

  if (settings->music_volume <= 0)
    settings->music_volume = 0;
  else if (settings->music_volume >= 100)
    settings->music_volume = 100;

  NXE::Sound::SoundManager::getInstance()->updateMusicVolume();
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _music_volume_get(ODItem *item)
{
  char str[10];
  sprintf(str, "%d%%", settings->music_volume);
  strcpy(item->suffix, str);
}


void _music_interpolation_change(ODItem *item, int dir)
{
  int result = settings->music_interpolation + dir;
  if (result < 0)
    result = 2;
  if (result > 2)
    result = 0;
  settings->music_interpolation = result;
}

void _music_interpolation_get(ODItem *item)
{
  static const char *strs[] = {"Nearest", "Linear", "Cubic"};
  strcpy(item->suffix, strs[settings->music_interpolation]);
}


/*
void c------------------------------() {}
*/

void _rumble_change(ODItem *item, int dir)
{
  settings->rumble ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
}

void _rumble_get(ODItem *item)
{
  static const char *strs[] = {"Off", "On"};
  strcpy(item->suffix, strs[settings->rumble]);
}


void _scheme_change(ODItem *item, int dir)
{
  settings->control_scheme ^= 1;
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
  opt.dlg->Refresh();
  if (settings->control_scheme)
  {
    ACCEPT_BUTTON = FIREKEY;
    DECLINE_BUTTON = JUMPKEY;
  }
  else
  {
    ACCEPT_BUTTON = JUMPKEY;
    DECLINE_BUTTON = FIREKEY;
  }
}

void _scheme_get(ODItem *item)
{
  static const char *strs[] = {"Jump", "Fire"};
  strcpy(item->suffix, strs[settings->control_scheme]);
}

void _scheme_get2(ODItem *item)
{
  static const char *strs[] = {"Fire", "Jump"};
  strcpy(item->suffix, strs[settings->control_scheme]);
}

static void _upd_control(ODItem *item)
{
  char keyname[64];

  in_action action = input_get_mapping(item->id);

  if (action.key != -1)
  {
    int keysym = action.key;
    snprintf(keyname, 64, "%s", SDL_GetKeyName((SDL_Keycode)keysym));
  }
  else if (action.jbut != -1)
  {
    snprintf(keyname, 64, "JBut %d", action.jbut);
  }
  else if (action.jaxis != -1)
  {
    if (action.jaxis_value > 0)
      snprintf(keyname, 64, "JAxis %d+", action.jaxis);
    else
      snprintf(keyname, 64, "JAxis %d-", action.jaxis);
  }
  else if (action.jhat != -1)
  {
    if (action.jhat_value & SDL_HAT_LEFT)
      snprintf(keyname, 64, "JHat %d L", action.jhat);
    else if (action.jhat_value & SDL_HAT_RIGHT)
      snprintf(keyname, 64, "JHat %d R", action.jhat);
    else if (action.jhat_value & SDL_HAT_UP)
      snprintf(keyname, 64, "JHat %d U", action.jhat);
    else if (action.jhat_value & SDL_HAT_DOWN)
      snprintf(keyname, 64, "JHat %d D", action.jhat);
  }

  maxcpy(item->righttext, _(keyname).c_str(), sizeof(item->righttext) - 1);
}

static void _edit_control(ODItem *item, int dir)
{
  Message *msg;

  opt.remapping_key     = item->id;
  opt.new_sdl_key.key   = -1;
  opt.new_sdl_key.jbut  = -1;
  opt.new_sdl_key.jhat  = -1;
  opt.new_sdl_key.jaxis = -1;

  msg               = new Message("Press new key for:", input_get_name(opt.remapping_key));
  msg->rawKeyReturn = &opt.new_sdl_key;
  msg->on_dismiss   = _finish_control_edit;

  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_DOOR);
}

static void _finish_control_edit(Message *msg)
{
  int inputno           = opt.remapping_key;
  in_action new_sdl_key = opt.new_sdl_key;
  int i;
  in_action action = input_get_mapping(inputno);

  // check if key is already in use
  for (i = 0; i < INPUT_COUNT; i++)
  {
    action = input_get_mapping(i);
    if (i != inputno && action.key != -1 && action.key == new_sdl_key.key)
    {
      new Message("Key already in use by:", input_get_name(i));
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
      return;
    }

    if (i != inputno && action.jbut != -1 && action.jbut == new_sdl_key.jbut)
    {
      new Message("Key already in use by:", input_get_name(i));
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
      return;
    }

    if (i != inputno && action.jhat != -1 && action.jhat == new_sdl_key.jhat
        && action.jhat_value & new_sdl_key.jhat_value)
    {
      new Message("Key already in use by:", input_get_name(i));
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
      return;
    }

    if (i != inputno && action.jaxis != -1 && action.jaxis == new_sdl_key.jaxis)
    {
      if (((action.jaxis_value > 0) && (new_sdl_key.jaxis_value > 0))
          || ((action.jaxis_value < 0) && (new_sdl_key.jaxis_value < 0)))
      {
        new Message("Key already in use by:", input_get_name(i));
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
        return;
      }
    }
  }

  input_remap(inputno, new_sdl_key);
  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
  opt.dlg->Refresh();
}
