
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "input.h"

struct Settings
{
  uint32_t version;
  int resolution;
  int last_save_slot;
  bool multisave;
  bool show_fps;
  bool fullscreen;

  bool enable_debug_keys;
  bool sound_enabled;
  int music_enabled;
  uint8_t new_music;

  bool skip_intro;
  bool rumble;
  bool strafing;

  in_action input_mappings[INPUT_COUNT];
  char language[256];
};

bool settings_load(Settings *settings = NULL);
bool settings_save(Settings *settings = NULL);

extern Settings *settings;
extern Settings normal_settings;

#endif
