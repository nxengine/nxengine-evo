
#include "../nx.h"
#include "options.h"
#include "dialog.h"
#include "message.h"
using namespace Options;
#include "../map.h"
#include "../settings.h"
#include "../input.h"
#include "../game.h"
#include "../sound/sound.h"
#include "../common/misc.h"
#include "../ResourceManager.h"

#include "../graphics/graphics.h"
#include "../graphics/font.h"

std::vector<void*> optionstack;

void DialogDismissed();
static void EnterMainMenu();
void LeavingMainMenu();
void _res_get(ODItem *item);
void _res_change(ODItem *item, int dir);
void _fullscreen_get(ODItem *item);
void _fullscreen_change(ODItem *item, int dir);

void _lang_get(ODItem *item);
void _lang_change(ODItem *item, int dir);

void _debug_change(ODItem *item, int dir);
void _debug_get(ODItem *item);
void _save_change(ODItem *item, int dir);
void _save_get(ODItem *item);
void _sound_change(ODItem *item, int dir);
void _sound_get(ODItem *item);
void _music_change(ODItem *item, int dir);
void _music_get(ODItem *item);
void _tracks_change(ODItem *item, int dir);
void _tracks_get(ODItem *item);
static void EnterControlsMenu(ODItem *item, int dir);
static void _upd_control(ODItem *item);
static void _edit_control(ODItem *item, int dir);
static void _finish_control_edit(Message *msg);


#define SLIDE_SPEED				32
#define SLIDE_DIST				(3 * SLIDE_SPEED)

static struct
{
	Dialog *dlg, *subdlg;
	Dialog *dismiss_on_focus;
	unsigned int mm_cursel;
	bool InMainMenu;
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
	
	inputs[F3KEY] = 0;
	sound(SND_MENU_MOVE);
	return 0;
}

void options_close()
{
	Options::close_objects();
	for (unsigned int i=0;i<optionstack.size();i++)
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

	if (justpushed(F3KEY))
	{
		game.pause(0);
		return;
	}
	
	Graphics::ClearScreen(BLACK);
	Options::run_and_draw_objects();
	
	fh = (FocusHolder *)optionstack.at(optionstack.size() - 1);
	if (fh)
	{
		fh->RunInput();
		if (game.paused != GP_OPTIONS) return;
		
		fh = (FocusHolder *)optionstack.at(optionstack.size() - 1);
		if (fh == opt.dismiss_on_focus && fh)
		{
			opt.dismiss_on_focus = NULL;
			delete fh;
//			optionstack.erase(optionstack.begin()+(optionstack.size() - 1));
		}
	}
	
	for(i=0;i<optionstack.size();i++)
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
	
	dlg->AddItem("Resolution: ", _res_change, _res_get, -1, OD_CHOICE);
	dlg->AddItem("Fullscreen: ", _fullscreen_change, _fullscreen_get, -1, OD_CHOICE);
	dlg->AddItem("Controls", EnterControlsMenu);
	
	dlg->AddSeparator();

#if defined(DEBUG)
	dlg->AddItem("Debug Keys: ", _debug_change, _debug_get, -1, OD_CHOICE);
	dlg->AddItem("Save Slots: ", _save_change, _save_get, -1, OD_CHOICE);
	
	dlg->AddSeparator();
#endif

	dlg->AddItem("Music: ", _music_change, _music_get, -1, OD_CHOICE);
	dlg->AddItem("Tracks: ", _tracks_change, _tracks_get, -1, OD_CHOICE);
	dlg->AddItem("Sound: ", _sound_change, _sound_get, -1, OD_CHOICE);

	if (game.mode == GM_TITLE)
		dlg->AddItem("Language: ", _lang_change, _lang_get, -1, OD_CHOICE);
	
	dlg->AddSeparator();
	dlg->AddDismissalItem();
	
	dlg->SetSelection(opt.mm_cursel);
	dlg->onclear = LeavingMainMenu;
	opt.InMainMenu = true;
}

void LeavingMainMenu()
{
	opt.mm_cursel = opt.dlg->GetSelection();
	opt.dlg->onclear = NULL;
	opt.InMainMenu = false;
}


void _res_get(ODItem *item)
{
	const gres_t *reslist = Graphics::GetRes();
	
	if (settings->resolution < 0 || \
		settings->resolution >= Graphics::GetResCount())
	{
		item->suffix[0] = 0;
	}
	else
	{
		strcpy(item->suffix, reslist[settings->resolution].name);
	}
}



void _res_change(ODItem *item, int dir)
{
int numres = Graphics::GetResCount();
int newres;

	sound(SND_DOOR);
	
	newres = (settings->resolution + dir);
	if (newres >= numres) newres = 1;
	if (newres < 1) newres = (numres - 1);
	const gres_t* res = Graphics::GetRes();
	while(!res[newres].enabled)
	{
	    newres += dir;
	    if (newres >= numres) newres = 1;
	    if (newres < 1) newres = (numres - 1);
	}
	
	if (!Graphics::SetResolution(newres, true))
	{
		settings->resolution = newres;
//		Graphics::SetFullscreen(false);
//		Graphics::SetFullscreen(settings->fullscreen);
//		opt.dlg->UpdateSizePos();
	}
	else
	{
		new Message("Resolution change failed");
		sound(SND_GUN_CLICK);
	}
}

void _lang_get(ODItem *item)
{
	std::vector<std::string> langs = ResourceManager::getInstance()->languages();
	for (auto &l: langs)
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
	int i = 0;
	for (auto &l: langs)
	{
	    if (l == settings->language)
	        break;
	    i++;
	}
	i+=dir;
	if (i<0)
	{
	    i = langs.size()-1;
	}
	if (i>=(int)langs.size())
	{
	    i = 0;
	}
	memset(settings->language,0,256);
    strncpy(settings->language,langs[i].c_str(), 255);
    game.lang->load();
    font_reload();
    game.tsc->Init();
    Graphics::FlushAll();
}


void _fullscreen_get(ODItem *item)
{
	static const char *strs[] = { "Off", "On" };
	strcpy(item->suffix, strs[settings->fullscreen]);
}


void _fullscreen_change(ODItem *item, int dir)
{
	settings->fullscreen ^= 1;
	sound(SND_MENU_SELECT);
	Graphics::SetFullscreen(settings->fullscreen);
}


void _debug_change(ODItem *item, int dir)
{
	settings->enable_debug_keys ^= 1;
	sound(SND_MENU_SELECT);
}

void _debug_get(ODItem *item)
{
	static const char *strs[] = { "Off", "On" };
	strcpy(item->suffix, strs[settings->enable_debug_keys]);
}


void _save_change(ODItem *item, int dir)
{
	settings->multisave ^= 1;
	sound(SND_MENU_MOVE);
}

void _save_get(ODItem *item)
{
	static const char *strs[] = { "1", "5" };
	strcpy(item->suffix, strs[settings->multisave]);
}


void _sound_change(ODItem *item, int dir)
{
	settings->sound_enabled ^= 1;
	sound(SND_MENU_SELECT);
}

void _sound_get(ODItem *item)
{
	static const char *strs[] = { "Off", "On" };
	strcpy(item->suffix, strs[settings->sound_enabled]);
}



void _music_change(ODItem *item, int dir)
{
	int result = settings->music_enabled + dir;
	if (result < 0) result = 2;
	if (result > 2) result = 0;
	music_set_enabled(result);
	sound(SND_MENU_SELECT);
}

void _music_get(ODItem *item)
{
	static const char *strs[] = { "Off", "On", "Boss Only" };
	strcpy(item->suffix, strs[settings->music_enabled]);
}

void _tracks_change(ODItem *item, int dir)
{
	int result = settings->new_music + dir;
	if (result < 0) result = 2;
	if (result > 2) result = 0;
	music_set_newmusic(result);
	sound(SND_MENU_SELECT);
}

void _tracks_get(ODItem *item)
{
	static const char *strs[] = { "Old", "New", "Remastered" };
	strcpy(item->suffix, strs[settings->new_music]);
}


/*
void c------------------------------() {}
*/

void _rumble_change(ODItem *item, int dir)
{
	settings->rumble ^= 1;
	sound(SND_MENU_SELECT);
}

void _rumble_get(ODItem *item)
{
	static const char *strs[] = { "Off", "On" };
	strcpy(item->suffix, strs[settings->rumble]);
}


static void EnterControlsMenu(ODItem *item, int dir)
{
Dialog *dlg = opt.dlg;

	dlg->Clear();
	sound(SND_MENU_MOVE);
	dlg->AddItem("Force feedback: ", _rumble_change, _rumble_get, -1, OD_CHOICE);
	dlg->AddItem("Left", _edit_control, _upd_control, LEFTKEY);
	dlg->AddItem("Right", _edit_control, _upd_control, RIGHTKEY);
	dlg->AddItem("Up", _edit_control, _upd_control, UPKEY);
	dlg->AddItem("Down", _edit_control, _upd_control, DOWNKEY);
	
//	dlg->AddSeparator();
	
	dlg->AddItem("Jump", _edit_control, _upd_control, JUMPKEY);
	dlg->AddItem("Fire", _edit_control, _upd_control,  FIREKEY);
	dlg->AddItem("Wpn Prev", _edit_control, _upd_control, PREVWPNKEY);
	dlg->AddItem("Wpn Next", _edit_control, _upd_control, NEXTWPNKEY);
	dlg->AddItem("Inventory", _edit_control, _upd_control, INVENTORYKEY);
	dlg->AddItem("Map", _edit_control, _upd_control, MAPSYSTEMKEY);
	dlg->AddItem("Pause", _edit_control, _upd_control, ESCKEY);
	
	dlg->AddSeparator();
	dlg->AddDismissalItem();
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
	    if (action.jaxis_value>0)
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
	
	maxcpy(item->righttext, keyname, sizeof(item->righttext) - 1);
}

static void _edit_control(ODItem *item, int dir)
{
Message *msg;

	opt.remapping_key = item->id;
	opt.new_sdl_key.key = -1;
	opt.new_sdl_key.jbut = -1;
	opt.new_sdl_key.jhat = -1;
	opt.new_sdl_key.jaxis = -1;
	
	msg = new Message("Press new key for:", input_get_name(opt.remapping_key));
	msg->rawKeyReturn = &opt.new_sdl_key;
	msg->on_dismiss = _finish_control_edit;
	
	sound(SND_DOOR);
}

static void _finish_control_edit(Message *msg)
{
	int inputno = opt.remapping_key;
	in_action new_sdl_key = opt.new_sdl_key;
	int i;
	in_action action = input_get_mapping(inputno);
	
	// check if key is already in use
	for(i=0;i<INPUT_COUNT;i++)
	{
	    action = input_get_mapping(i);
		if (i != inputno && action.key!=-1 && action.key == new_sdl_key.key)
		{
			new Message("Key already in use by:", input_get_name(i));
			sound(SND_GUN_CLICK);
			return;
		}

		if (i != inputno && action.jbut!=-1 && action.jbut == new_sdl_key.jbut)
		{
			new Message("Key already in use by:", input_get_name(i));
			sound(SND_GUN_CLICK);
			return;
		}

		if (i != inputno && action.jhat!=-1 && action.jhat == new_sdl_key.jhat && action.jhat_value & new_sdl_key.jhat_value)
		{
			new Message("Key already in use by:", input_get_name(i));
			sound(SND_GUN_CLICK);
			return;
		}

		if (i != inputno && action.jaxis!=-1 && action.jaxis == new_sdl_key.jaxis)
		{
		    if ( ((action.jaxis_value > 0) && (new_sdl_key.jaxis_value > 0)) || ((action.jaxis_value < 0) && (new_sdl_key.jaxis_value < 0)))
		    {
			    new Message("Key already in use by:", input_get_name(i));
			    sound(SND_GUN_CLICK);
			    return;
			}
		}


	}
	
	input_remap(inputno, new_sdl_key);
	sound(SND_MENU_SELECT);
	opt.dlg->Refresh();
}














