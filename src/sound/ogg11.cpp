
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

#include "../common/basics.h"
#include "ogg11.h"
#include "sound.h"			// SAMPLE_RATE
#include "../common/stat.h"
#include "../common/misc.h"
#include "../ResourceManager.h"

static ogg11Song song;
static bool do_loop = false;
static bool looped = false;

bool ogg11_load(const std::string& fname, const std::string& dir)
{
    std::string filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + "_intro.ogg");
    if (!ResourceManager::fileExists(filename))
    {
        filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + "_loop.ogg");
        if (!ResourceManager::fileExists(filename))
        {
            filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + ".ogg");
        }
    }
	song.intro=NULL;
	song.intro=Mix_LoadMUS(filename.c_str());
	if (!song.intro)
	{
		staterr("Mix_LoadMUS(): %s\n", Mix_GetError());
		return false;
	}

    filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + "_loop.ogg");
    if (!ResourceManager::fileExists(filename))
    {
        filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + ".ogg");
    }

	song.loop=NULL;
	song.loop=Mix_LoadMUS(filename.c_str());
	if (!song.loop)
	{
		staterr("Mix_LoadMUS(): %s\n", Mix_GetError());
		return false;
	}
	
	if (ResourceManager::fileExists(ResourceManager::getInstance()->getLocalizedPath(dir + fname + ".noloop")))
	{
		song.doloop = false;
	}
	else
	{
		song.doloop = true;
	}
	
	return true;
}

void musicFinished11()
{
	// Here be dragons
	// SDL_mixer documantation states that we should never call SDL_mixer functions from callback
	// However, using flag/event/whatever to restart music in main loop
	// produces audible pause.
	// This seems to work, but needs extensive testing.

	looped = true;
	song.last_pos = SDL_GetTicks();
	if (song.doloop)
	{
		Mix_PlayMusic(song.loop,0);
	}
}


// start the currently-loaded track playing at beat startbeat.
bool ogg11_start(const std::string& fname, const std::string& dir, int startbeat, bool loop)
{
	ogg11_stop();		// stop any old music
	
	if (!ogg11_load(fname, dir))
	{
	    return false;
		song.playing = false;
	}
	
	song.last_pos = 0;
	
	song.playing = true;
	song.fading = false;
	
	song.volume = 75;
	
	do_loop = false;
	
	looped = loop;
	
	song.last_pos = SDL_GetTicks()-startbeat;
	
	if (looped)
	{
		Mix_PlayMusic(song.loop,0);
	}
	else
	{
		Mix_PlayMusic(song.intro,0);
	}
	Mix_VolumeMusic(song.volume);
	Mix_SetMusicPosition(startbeat / 1000);
	Mix_HookMusicFinished(musicFinished11);
	return true;
}


// pause/stop playback of the current song
void ogg11_stop(void)
{
	extern int lastsongpos;
	extern bool songlooped;
	if (song.playing)
	{
		song.playing = false;
		do_loop = false;
		songlooped = looped;
		looped = false;
		Mix_HookMusicFinished(NULL);
		Mix_HaltMusic();
		if (song.intro)
		{
			Mix_FreeMusic(song.intro);
			song.intro=NULL;
		}
		if (song.loop)
		{
			Mix_FreeMusic(song.loop);
			song.loop=NULL;
		}
		lastsongpos = SDL_GetTicks()-song.last_pos;
	}
}

bool ogg11_is_playing(void)
{
	return song.playing;
}

void ogg11_fade(void)
{
	song.fading = true;
	song.last_fade_time = 0;
}

void ogg11_set_volume(int newvolume)
{
	if (newvolume != song.volume)
	{
		song.volume = newvolume;
		Mix_VolumeMusic(song.volume);
	}
}

void ogg11_run_loop()
{
/*    if (do_loop)
    {
        printf("looped\n");
        do_loop = false;
        song.last_pos = SDL_GetTicks();
        Mix_PlayMusic(song.loop,0);
    }
*/
}

void ogg11_run_fade()
{
	ogg11_run_loop();
	if (!song.fading) return;
	uint32_t curtime = SDL_GetTicks();
	if ((curtime - song.last_fade_time) >= 25)
	{
		int newvol = (song.volume - 1);
		if (newvol <= 0)
		{
			song.fading = false;
			ogg11_stop();
		}
		else
		{
			ogg11_set_volume(newvol);
		}
		
		song.last_fade_time = curtime;
	}
}

