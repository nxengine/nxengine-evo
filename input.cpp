
#include "nx.h"
#include "input.h"
#include "console.h"
#include "game.h"
#include "sound/sound.h"
#include "common/stat.h"

in_action mappings[INPUT_COUNT];

bool inputs[INPUT_COUNT];
bool lastinputs[INPUT_COUNT];
in_action last_sdl_action;
SDL_Joystick *joy;
SDL_Haptic *haptic;

bool input_init(void)
{
	memset(inputs, 0, sizeof(inputs));
	memset(lastinputs, 0, sizeof(lastinputs));
	memset(mappings, 0xff, sizeof(mappings));
	
	// default mappings
	mappings[LEFTKEY].key      = SDLK_LEFT;
	mappings[RIGHTKEY].key     = SDLK_RIGHT;
	mappings[UPKEY].key        = SDLK_UP;
	mappings[DOWNKEY].key      = SDLK_DOWN;
	mappings[JUMPKEY].key      = SDLK_z;
	mappings[FIREKEY].key      = SDLK_x;
	mappings[PREVWPNKEY].key   = SDLK_a;
	mappings[NEXTWPNKEY].key   = SDLK_s;
	mappings[INVENTORYKEY].key = SDLK_q;
	mappings[MAPSYSTEMKEY].key = SDLK_w;
	
	mappings[ESCKEY].key = SDLK_ESCAPE;
	
	mappings[F1KEY].key  = SDLK_F1;
	mappings[F2KEY].key  = SDLK_F2;
	mappings[F3KEY].key  = SDLK_F3;
	mappings[F4KEY].key  = SDLK_F4;
	mappings[F5KEY].key  = SDLK_F5;
	mappings[F6KEY].key  = SDLK_F6;
	mappings[F7KEY].key  = SDLK_F7;
	mappings[F8KEY].key  = SDLK_F8;
	mappings[F9KEY].key  = SDLK_F9;
	mappings[F10KEY].key = SDLK_F10;
	mappings[F11KEY].key = SDLK_F11;
	mappings[F12KEY].key = SDLK_F12;
	
	mappings[FREEZE_FRAME_KEY].key  = SDLK_SPACE;
	mappings[FRAME_ADVANCE_KEY].key = SDLK_c;
	mappings[DEBUG_FLY_KEY].key     = SDLK_v;

	mappings[HOMEKEY].key  = SDLK_HOME;
	mappings[ENDKEY].key   = SDLK_END;
	mappings[ENTERKEY].key = SDLK_RETURN;
	
	SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
	if (SDL_NumJoysticks() > 0) {
	    // Open joystick
	    joy = SDL_JoystickOpen(0);
	
	    if (joy) {
	        stat("Opened Joystick 0");
	        stat("Name: %s", SDL_JoystickNameForIndex(0));
	        stat("Number of Axes: %d", SDL_JoystickNumAxes(joy));
	        stat("Number of Buttons: %d", SDL_JoystickNumButtons(joy));
	        stat("Number of Balls: %d", SDL_JoystickNumBalls(joy));
	        haptic = SDL_HapticOpenFromJoystick( joy );
	        if (haptic == NULL)
	        {
	            stat("No force feedback support");
	        }
	        else
	        {
	            if (SDL_HapticRumbleInit( haptic ) != 0)
	                stat("Coiuldn't init simple rumble");
	        }
	    } else {
	        stat("Couldn't open Joystick 0");
	    }
	                                                                    
	}
	return 0;
}

void rumble(float str, uint32_t len)
{
    if (haptic != NULL)
        SDL_HapticRumblePlay(haptic, str, len);
}


// set the SDL key that triggers an input
void input_remap(int keyindex, int32_t sdl_key)
{
	stat("input_remap(%d => %d)", keyindex, sdl_key);
	in_action old_mapping = input_get_mapping(keyindex);
//	if (old_mapping != -1)
//		mappings[old_mapping] = 0xff;
	
	mappings[keyindex].key = sdl_key;
}

// get which SDL key triggers a given input
in_action input_get_mapping(int keyindex)
{
  return mappings[keyindex];
}

int input_get_action(int32_t sdlkey)
{
  for (int i=0;i<INPUT_COUNT;i++)
  {
    if(mappings[i].key == sdlkey)
    {
      return i;
    }
  }
  return -1;
}


const char *input_get_name(int index)
{
static const char *input_names[] =
{
	"left", "right", "up", "down",
	"jump", "fire", "pervious wpn", "next wpn",
	"inventory", "map",
	"escape",
	"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
	"freeze frame", "frame advance", "debug fly"
};

	if (index < 0 || index >= INPUT_COUNT)
		return "invalid";
	
	return input_names[index];
}

void input_set_mappings(in_action *array)
{
	memset(mappings, 0xff, sizeof(mappings));
	for(int i=0;i<INPUT_COUNT;i++)
		mappings[i] = array[i];
}

/*
void c------------------------------() {}
*/

// keys that we don't want to send to the console
// even if the console is up.
static int IsNonConsoleKey(int key)
{
static const int nosend[] = { SDLK_LEFT, SDLK_RIGHT, 0 };

	for(int i=0;nosend[i];i++)
		if (key == nosend[i])
			return true;
	
	return false;
}

void input_poll(void)
{
SDL_Event evt;
int32_t key;
int ino;//, key;
	
	while(SDL_PollEvent(&evt))
	{
		switch(evt.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				key = evt.key.keysym.sym;
				
				static uint8_t shiftstates = 0;
				extern bool freezeframe;
				
				if (console.IsVisible() && !IsNonConsoleKey(key))
				{
					if (key == SDLK_LSHIFT)
					{
						if (evt.type == SDL_KEYDOWN)
							shiftstates |= LEFTMASK;
						else
							shiftstates &= ~LEFTMASK;
					}
					else if (key == SDLK_RSHIFT)
					{
						if (evt.type == SDL_KEYDOWN)
							shiftstates |= RIGHTMASK;
						else
							shiftstates &= ~RIGHTMASK;
					}
					else
					{
						int ch = key;
						if (shiftstates != 0)
						{
							ch = toupper(ch);
							if (ch == '.') ch = '>';
							if (ch == '-') ch = '_';
							if (ch == '/') ch = '?';
							if (ch == '1') ch = '!';
						}
						
						if (evt.type == SDL_KEYDOWN)
							console.HandleKey(ch);
						else
							console.HandleKeyRelease(ch);
					}
				}
				else
				{
					ino = input_get_action(key);//mappings[key];
					
					if (ino != -1)
						inputs[ino] = (evt.type == SDL_KEYDOWN);
					
					if (evt.type == SDL_KEYDOWN)
					{
						if (key == '`')		// bring up console
						{
							if (!freezeframe)
							{
								sound(SND_SWITCH_WEAPON);
								console.SetVisible(true);
							}
						}
						else
						{
							last_sdl_action.key = key;
						}
					}
				}
			}
			break;
			
			case SDL_QUIT:
			{
				inputs[ESCKEY] = true;
				game.running = false;
			}
			break;
			
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			{
			    Uint8 but = evt.jbutton.button;
			    stat(">>>> joy button: %d = %d", but, (evt.jbutton.state == SDL_PRESSED));
			    inputs[but+4] = (evt.jbutton.state == SDL_PRESSED);

			}
			break;
			
			case SDL_JOYHATMOTION:
			{
			    stat(">>>> joy hat: %d = %d", evt.jhat.hat, evt.jhat.value);
			    inputs[LEFTKEY] = evt.jhat.value & SDL_HAT_LEFT;
			    inputs[RIGHTKEY] = evt.jhat.value & SDL_HAT_RIGHT;
			    inputs[UPKEY] = evt.jhat.value & SDL_HAT_UP;
			    inputs[DOWNKEY] = evt.jhat.value & SDL_HAT_DOWN;
			}
			break;

			case SDL_JOYAXISMOTION:
			{
			    stat(">>>> joy axis: %d = %d", evt.jaxis.axis, evt.jaxis.value);
			}
			break;

		}
	}
}



void input_close(void)
{
    // Close if opened
    if (SDL_JoystickGetAttached(joy)) {
        SDL_JoystickClose(joy);
    }
}

/*
void c------------------------------() {}
*/

static const int buttons[] = { JUMPKEY, FIREKEY, 0 };

bool buttondown(void)
{
	for(int i=0;buttons[i];i++)
	{
		if (inputs[buttons[i]])
			return 1;
	}
	
	return 0;
}

bool buttonjustpushed(void)
{
	for(int i=0;buttons[i];i++)
	{
		if (inputs[buttons[i]] && !lastinputs[buttons[i]])
			return 1;
	}
	
	return 0;
}

bool justpushed(int k)
{
	return (inputs[k] && !lastinputs[k]);
}





