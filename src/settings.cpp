
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include "settings.h"
#include "input.h"
#include "ResourceManager.h"
#include "common/stat.h"
#include "common/misc.h"

const uint32_t SETTINGS_VERSION = ( ( '3' << 24 ) + ( 'S' << 16 ) + ( 'X' << 8 ) + 'N' );		// serves as both a version and magic

Settings normal_settings;
Settings *settings = &normal_settings;

static bool tryload(Settings *setfile)
{
FILE *fp;

	char* basepath = SDL_GetPrefPath("nxengine", "nxengine-evo");
	std::string path = std::string(basepath) + "settings.dat";
	SDL_free(basepath);

	stat("Loading settings...");
	
	fp = myfopen(widen(path).c_str(), widen("rb").c_str());
	if (!fp)
	{
		stat("Couldn't open file %s.", path.c_str());
		return 1;
	}
	
	setfile->version = 0;
	fread(setfile, sizeof(Settings), 1, fp);
	if (setfile->version != SETTINGS_VERSION)
	{
		stat("Wrong settings version %04x.", setfile->version);
		fclose(fp);
		return 1;
	}
	
	fclose(fp);
	std::vector<std::string> langs = ResourceManager::getInstance()->languages();
	bool found = false;
	for (auto &l: langs)
	{
		if (strcmp(settings->language, l.c_str()) == 0)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		memset(setfile->language,0,256);
		strncpy(setfile->language,"english", 255);
	}

	return 0;
}

bool settings_load(Settings *setfile)
{
	if (!setfile) setfile = &normal_settings;
	
	if (tryload(settings))
	{
		stat("No saved settings; using defaults.");
		
		memset(setfile, 0, sizeof(Settings));
		setfile->resolution = 2;		// 640x480 Windowed, should be safe value
		setfile->last_save_slot = 0;
		setfile->multisave = true;
		setfile->fullscreen = false;
		
#if defined(DEBUG)
		setfile->enable_debug_keys = true;
#else
		setfile->enable_debug_keys = false;
#endif
		setfile->sound_enabled = true;
		setfile->music_enabled = 1;	// both Boss and Regular music
		setfile->new_music = 0;
		setfile->rumble = false;
		memset(setfile->language,0,256);
		strncpy(setfile->language,"english", 255);
		
		return 1;
	}
	else
	{
		input_set_mappings(settings->input_mappings);
#if defined(DEBUG)
		setfile->enable_debug_keys = true;
#else
		setfile->enable_debug_keys = false;
#endif
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/


bool settings_save(Settings *setfile)
{
FILE *fp;

	char* basepath = SDL_GetPrefPath("nxengine", "nxengine-evo");
	std::string path = std::string(basepath) + "settings.dat";
	SDL_free(basepath);

	if (!setfile)
		setfile = &normal_settings;
	
	stat("Writing settings...");
	fp = myfopen(widen(path).c_str(), widen("wb").c_str());
	if (!fp)
	{
		stat("Couldn't open file %s.", path.c_str());
		return 1;
	}
	
	for(int i=0;i<INPUT_COUNT;i++)
		setfile->input_mappings[i] = input_get_mapping(i);
	
	setfile->version = SETTINGS_VERSION;
	fwrite(setfile, sizeof(Settings), 1, fp);
	
	fclose(fp);
	return 0;
}




