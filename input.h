
#ifndef _INPUT_H
#define _INPUT_H
#include <SDL.h>

typedef struct 
{
    int32_t key;
    int32_t hat;
    int32_t hat_value;
    int32_t axis;
    int32_t axis_value;
} in_action;

enum INPUTS
{
	LEFTKEY, RIGHTKEY, UPKEY, DOWNKEY,
	JUMPKEY, FIREKEY,
	PREVWPNKEY, NEXTWPNKEY,
	INVENTORYKEY, MAPSYSTEMKEY,
	
	ESCKEY,
	F1KEY,
	F2KEY,
	F3KEY,
	F4KEY,
	F5KEY,
	F6KEY,
	F7KEY,
	F8KEY,
	F9KEY,
	F10KEY,
	F11KEY,
	F12KEY,
	
	FREEZE_FRAME_KEY,
	FRAME_ADVANCE_KEY,
	DEBUG_FLY_KEY,
	HOMEKEY,
	ENDKEY,
	ENTERKEY,
	
	INPUT_COUNT
};

#define LASTCONTROLKEY		MAPSYSTEMKEY

#define DEBUG_GOD_KEY		F1KEY
#define DEBUG_MOVE_KEY		F2KEY
#define DEBUG_SAVE_KEY		F4KEY
#define FFWDKEY				F5KEY

extern bool inputs[INPUT_COUNT];
extern bool lastinputs[INPUT_COUNT];
extern int32_t last_sdl_key;
extern SDL_Joystick *joy;
extern SDL_Haptic *haptic;

bool input_init(void);
void input_remap(int keyindex, int32_t sdl_key);
in_action input_get_mapping(int keyindex);
const char *input_get_name(int index);
void input_set_mappings(in_action *array);
void input_poll(void);
void input_close(void);
bool buttondown(void);
bool buttonjustpushed(void);
bool justpushed(int k);
void rumble(float str, uint32_t len);

#endif
