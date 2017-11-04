
/* SOUND.C
  PXT/SS/Org sound interface
*/
#include <stdio.h>
#include <string.h>
#include <string>
#include <SDL_mixer.h>

#include "../nx.h"
#include "../settings.h"
#include "../game.h"
#include "pxt.h"
#include "org.h"
//#include "sslib.h"
#include "sound.h"
#include "../common/stat.h"

#define MUSIC_OFF		0
#define MUSIC_ON		1
#define MUSIC_BOSS_ONLY	2
static int lastsong = 0;		// this holds the previous song, for <RMU
static int cursong = 0;
int lastsongpos = 0;

// there are more than this around 9b; those are drums and are loaded by the org module
#define NUM_SOUNDS		0x75
#define ORG_VOLUME		75

const char *org_names[] =
{
	NULL,
	"wanpaku", "anzen", "gameover", "gravity", "weed", "mdown2", "fireeye",
	"vivi", "mura", "fanfale1", "ginsuke", "cemetery", "plant", "kodou", "fanfale3",
	"fanfale2", "dr", "escape", "jenka", "maze", "access", "ironh", "grand",
	"curly", "oside", "requiem", "wanpak2", "quiet", "lastcave", "balcony", "lastbtl",
	"lastbt3", "ending", "zonbie", "bdown", "hell", "jenka2", "marine", "ballos",
	"toroko", "white", NULL
};

static const char bossmusic[] = { 4, 7, 10, 11, 15, 16, 17, 18, 21, 22, 31, 33, 35, 0 };

static const char *pxt_dir = "./data/pxt/";
static const char *org_dir = "./data/org/";
static const char *org_wavetable = "./data/wavetable.dat";

bool sound_init(void)
{
	char* basepath = SDL_GetPrefPath("nxengine", "nxengine-evo");
	std::string sndcache = std::string(basepath) + "sndcache.pcm";
	SDL_free(basepath);
//	if (SSInit()) return 1;
	if (Mix_Init(MIX_INIT_OGG) == -1)
	{
		staterr("Unable to init mixer.");
		return 1;
	}
	if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16, 2, 2048) == -1)
	{
		staterr("Unable to init mixer.");
		return 1;
	}
	
	Mix_AllocateChannels(64);
	
	Mix_ChannelFinished(pxtSoundDone);
	
	if (pxt_init()) return 1;
	
	if (pxt_LoadSoundFX(pxt_dir, sndcache.c_str(), NUM_SOUNDS)) return 1;
	
	if (org_init(org_wavetable, pxt_dir, ORG_VOLUME))
	{
		staterr("Music failed to initialize");
		return 1;
	}
	
	return 0;
}

void sound_close(void)
{
	pxt_freeSoundFX();
	Mix_CloseAudio();
	Mix_Quit();
//	SSClose();
}

/*
void c------------------------------() {}
*/

void sound(int snd)
{
	if (!settings->sound_enabled)
		return;
	
	pxt_Stop(snd);
	pxt_Play(-1, snd, 0);
}

void sound_loop(int snd)
{
	if (!settings->sound_enabled)
		return;
	
	pxt_Play(-1, snd, -1);
}

void sound_loop_resampled(int snd, int percent)
{
	if (!settings->sound_enabled)
		return;
	
	pxt_PlayResampled(-1, snd, -1, percent);
}

void sound_stop(int snd)
{
	pxt_Stop(snd);
}

bool sound_is_playing(int snd)
{
	return pxt_IsPlaying(snd);
}


void StartStreamSound(int percent)
{
	sound_loop_resampled(SND_STREAM1, percent);
	sound_loop_resampled(SND_STREAM2, percent+100);
}

void StartPropSound(void)
{
	sound_loop(SND_PROPELLOR);
}

void StopLoopSounds(void)
{
	sound_stop(SND_STREAM1);
	sound_stop(SND_STREAM2);
	sound_stop(SND_PROPELLOR);
}

/*
void c------------------------------() {}
*/

static void start_track(int songno, int pos)
{
char fname[MAXPATHLEN];

	if (songno == 0)
	{
		org_stop();
		return;
	}
	
	strcpy(fname, org_dir);
	strcat(fname, org_names[songno]);
	strcat(fname, ".org");
	
	if (!org_load(fname))
	{
		org_start(pos);
	}
}


void music(int songno, int pos)
{
	if (songno == cursong)
		return;
	
	lastsong = cursong;
	cursong = songno;
	
	stat(" >> music(%d)", songno);
	
	if (songno != 0 && !should_music_play(songno, settings->music_enabled))
	{
		stat("Not playing track %d because music_enabled is %d", songno, settings->music_enabled);
		org_stop();
		return;
	}
	
	start_track(songno,pos);
}


bool should_music_play(int songno, int musicmode)
{
	if (game.mode == GM_TITLE || game.mode == GM_CREDITS)
		return true;
	
	switch(musicmode)
	{
		case MUSIC_OFF: return false;
		case MUSIC_ON:  return true;
		case MUSIC_BOSS_ONLY:
			return music_is_boss(songno);
	}
	
	return false;
}

bool music_is_boss(int songno)
{
	if (strchr(bossmusic, songno))
		return true;
	else
		return false;
}

void music_set_enabled(int newstate)
{
	if (newstate != settings->music_enabled)
	{
		stat("music_set_enabled(%d)", newstate);
		
		settings->music_enabled = newstate;
		bool play = should_music_play(cursong, newstate);
		
		if (play != org_is_playing())
		{
			if (play)
				start_track(cursong,0);
			else
				org_stop();
		}
	}
}

void music_fade() { org_fade(); }

int music_cursong()		{ return cursong; }
int music_lastsong() 	{ return lastsong; }
int music_lastsongpos() 	{ return lastsongpos; }
void music_run_fade() { org_run_fade(); }

