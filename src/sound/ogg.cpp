
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

#include "../common/basics.h"
#include "ogg.h"
#include "sound.h"			// SAMPLE_RATE
#include "../common/stat.h"
#include "../common/misc.h"

static oggSong song;
static bool do_loop = 0;

char ogg_load(const std::string& fname)
{
	song.music=NULL;
	song.music=Mix_LoadMUS(fname.c_str());
	if (!song.music)
	{
		staterr("Mix_LoadMUS(): %s\n", Mix_GetError());
		return 1;
	}

	return 0;
}

void musicFinished()
{
	// We shouldn't call sdl_mixer functions from callback
	// thus, set a flag and restart music on next loop
	do_loop = true;
}


// start the currently-loaded track playing at beat startbeat.
bool ogg_start(const std::string& fname, int startbeat)
{
	ogg_stop();		// stop any old music
	
	ogg_load(fname);
	
	song.last_pos = 0;
	
	song.playing = true;
	song.fading = false;
	
	song.volume = 75;
	
	do_loop = false;
	
	song.last_pos = SDL_GetTicks()-startbeat;
	
	Mix_PlayMusic(song.music,0);
	Mix_VolumeMusic(song.volume);
	Mix_SetMusicPosition(startbeat / 1000);
	Mix_HookMusicFinished(musicFinished);
	return 0;
}


// pause/stop playback of the current song
void ogg_stop(void)
{
	extern int lastsongpos;
	if (song.playing)
	{
		song.playing = false;
		do_loop = false;
		Mix_HookMusicFinished(NULL);
		Mix_HaltMusic();
		if (song.music)
		{
			Mix_FreeMusic(song.music);
			song.music=NULL;
		}
		lastsongpos = SDL_GetTicks()-song.last_pos;
		printf("lastsongpos = %d\n",lastsongpos);
	}
}

bool ogg_is_playing(void)
{
	return song.playing;
}

void ogg_fade(void)
{
	song.fading = true;
	song.last_fade_time = 0;
}

void ogg_set_volume(int newvolume)
{
	if (newvolume != song.volume)
	{
		song.volume = newvolume;
		Mix_VolumeMusic(song.volume);
	}
}

void ogg_run_loop()
{
    if (do_loop)
    {
        do_loop = false;
        song.last_pos = SDL_GetTicks();
        Mix_PlayMusic(song.music,0);
    }
}

void ogg_run_fade()
{
	ogg_run_loop();
	if (!song.fading) return;
	uint32_t curtime = SDL_GetTicks();
	if ((curtime - song.last_fade_time) >= 25)
	{
		int newvol = (song.volume - 1);
		if (newvol <= 0)
		{
			song.fading = false;
			ogg_stop();
		}
		else
		{
			ogg_set_volume(newvol);
		}
		
		song.last_fade_time = curtime;
	}
}

