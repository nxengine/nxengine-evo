
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "settings.h"
#include "input.h"
#include "common/stat.h"


const char *setfilename = "settings.dat";
const uint32_t SETTINGS_VERSION = ( ( '1' << 24 ) + ( 'S' << 16 ) + ( 'X' << 8 ) + 'N' );		// serves as both a version and magic

Settings normal_settings;
Settings *settings = &normal_settings;

static bool tryload(Settings *setfile)
{
FILE *fp;

	stat("Loading settings...");
	
	fp = fopen(setfilename, "rb");
	if (!fp)
	{
		stat("Couldn't open file %s.", setfilename);
		return 1;
	}
	
	setfile->version = 0;
	fread(setfile, sizeof(Settings), 1, fp);
	if (setfile->version != SETTINGS_VERSION)
	{
		stat("Wrong settings version %04x.", setfile->version);
		return 1;
	}
	
	fclose(fp);
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
		
		setfile->enable_debug_keys = false;
		setfile->sound_enabled = true;
		setfile->music_enabled = 1;	// both Boss and Regular music
		setfile->rumble = false;
		
		return 1;
	}
	else
	{
		#ifdef __SDLSHIM__
			stat("settings_load(): Hey FIXME!!!");
			settings->show_fps = true;
		#else
			input_set_mappings(settings->input_mappings);
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

	if (!setfile)
		setfile = &normal_settings;
	
	stat("Writing settings...");
	fp = fopen(setfilename, "wb");
	if (!fp)
	{
		stat("Couldn't open file %s.", setfilename);
		return 1;
	}
	
	for(int i=0;i<INPUT_COUNT;i++)
		setfile->input_mappings[i] = input_get_mapping(i);
	
	setfile->version = SETTINGS_VERSION;
	fwrite(setfile, sizeof(Settings), 1, fp);
	
	fclose(fp);
	return 0;
}




