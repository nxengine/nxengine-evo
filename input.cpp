
#include "nx.h"
#include "input.h"
#include "console.h"
#include "game.h"
#include "sound/sound.h"
#include "common/stat.h"

int32_t mappings[INPUT_COUNT];

bool inputs[INPUT_COUNT];
bool lastinputs[INPUT_COUNT];
int32_t last_sdl_key;

bool input_init(void)
{
	memset(inputs, 0, sizeof(inputs));
	memset(lastinputs, 0, sizeof(lastinputs));
	memset(mappings, 0xff, sizeof(mappings));
	
	// default mappings
	mappings[LEFTKEY]      = SDLK_LEFT;
	mappings[RIGHTKEY]     = SDLK_RIGHT;
	mappings[UPKEY]        = SDLK_UP;
	mappings[DOWNKEY]      = SDLK_DOWN;
	mappings[JUMPKEY]      = SDLK_z;
	mappings[FIREKEY]      = SDLK_x;
	mappings[PREVWPNKEY]   = SDLK_a;
	mappings[NEXTWPNKEY]   = SDLK_s;
	mappings[INVENTORYKEY] = SDLK_q;
	mappings[MAPSYSTEMKEY] = SDLK_w;
	
	mappings[ESCKEY] = SDLK_ESCAPE;
	
	mappings[F1KEY]  = SDLK_F1;
	mappings[F2KEY]  = SDLK_F2;
	mappings[F3KEY]  = SDLK_F3;
	mappings[F4KEY]  = SDLK_F4;
	mappings[F5KEY]  = SDLK_F5;
	mappings[F6KEY]  = SDLK_F6;
	mappings[F7KEY]  = SDLK_F7;
	mappings[F8KEY]  = SDLK_F8;
	mappings[F9KEY]  = SDLK_F9;
	mappings[F10KEY] = SDLK_F10;
	mappings[F11KEY] = SDLK_F11;
	mappings[F12KEY] = SDLK_F12;
	
	mappings[FREEZE_FRAME_KEY]  = SDLK_SPACE;
	mappings[FRAME_ADVANCE_KEY] = SDLK_c;
	mappings[DEBUG_FLY_KEY]     = SDLK_v;

	mappings[HOMEKEY]  = SDLK_HOME;
	mappings[ENDKEY]   = SDLK_END;
	mappings[ENTERKEY] = SDLK_RETURN;
	
	return 0;
}


// set the SDL key that triggers an input
void input_remap(int keyindex, int32_t sdl_key)
{
	stat("input_remap(%d => %d)", keyindex, sdl_key);
	int old_mapping = input_get_mapping(keyindex);
//	if (old_mapping != -1)
//		mappings[old_mapping] = 0xff;
	
	mappings[keyindex] = sdl_key;
}

// get which SDL key triggers a given input
int32_t input_get_mapping(int keyindex)
{
  return mappings[keyindex];
}

int input_get_action(int32_t sdlkey)
{
  for (int i=0;i<INPUT_COUNT;i++)
  {
    if(mappings[i] == sdlkey)
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

void input_set_mappings(int32_t *array)
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
							last_sdl_key = key;
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
		}
	}
}



void input_close(void)
{

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





