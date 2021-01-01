
#include "settings.h"

#include "ResourceManager.h"
#include "common/misc.h"
#include "Utils/Logger.h"
#include "input.h"

#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const uint32_t SETTINGS_VERSION = (('7' << 24) + ('S' << 16) + ('X' << 8) + 'N'); // serves as both a version and magic

Settings normal_settings;
Settings *settings = &normal_settings;

static bool tryload(Settings *setfile)
{
  FILE *fp;

  std::string path = ResourceManager::getInstance()->getPrefPath("settings.dat");

  LOG_INFO("Loading settings...");

  fp = myfopen(widen(path).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("Couldn't open file {}.", path);
    return 1;
  }

  setfile->version = 0;
  fread(setfile, sizeof(Settings), 1, fp);
  if (setfile->version != SETTINGS_VERSION)
  {
    LOG_ERROR("Wrong settings version {:#04x}.", setfile->version);
    fclose(fp);
    return 1;
  }

  fclose(fp);
  std::vector<std::string> langs = ResourceManager::getInstance()->languages();
  bool found                     = false;
  for (auto &l : langs)
  {
    if (strcmp(settings->language, l.c_str()) == 0)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    memset(setfile->language, 0, 256);
    strncpy(setfile->language, "english", 255);
  }

#if defined(__VITA__) || defined(__SWITCH__)
    setfile->resolution     = 1;
#endif

  return 0;
}

bool settings_load(Settings *setfile)
{
  if (!setfile)
    setfile = &normal_settings;

  if (tryload(settings))
  {
    LOG_INFO("No saved settings; using defaults.");

    memset(setfile, 0, sizeof(Settings));
#if defined(__VITA__) || defined(__SWITCH__)
    setfile->resolution     = 1;
#else
    setfile->resolution     = 2; // 640x480 Windowed, should be safe value
#endif
    setfile->last_save_slot = 0;
    setfile->fullscreen     = false;

    setfile->sound_enabled = true;
    setfile->music_enabled = 1; // both Boss and Regular music
    setfile->new_music     = 0;
    setfile->rumble        = false;
    setfile->sfx_volume = 100;
    setfile->music_volume = 100;
    setfile->animated_facepics = true;
    setfile->lights = false;
    setfile->control_scheme = false;
    memset(setfile->language, 0, 256);
    strncpy(setfile->language, "english", 255);

    return 1;
  }
  else
  {
    input_set_mappings(settings->input_mappings);
  }

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

  return 0;
}

/*
void c------------------------------() {}
*/

bool settings_save(Settings *setfile)
{
  FILE *fp;

  std::string path = ResourceManager::getInstance()->getPrefPath("settings.dat");

  if (!setfile)
    setfile = &normal_settings;

  LOG_INFO("Writing settings...");
  fp = myfopen(widen(path).c_str(), widen("wb").c_str());
  if (!fp)
  {
    LOG_ERROR("Couldn't open file {}.", path);
    return 1;
  }

  for (int i = 0; i < INPUT_COUNT; i++)
    setfile->input_mappings[i] = input_get_mapping(i);

  setfile->version = SETTINGS_VERSION;
  fwrite(setfile, sizeof(Settings), 1, fp);

  fclose(fp);
  return 0;
}
