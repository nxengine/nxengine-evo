
/* SOUND.C
  PXT/SS/Org sound interface
*/
#include <cstdio>
#include <cstring>
#include <string>
#include <SDL_mixer.h>

#include "../nx.h"
#include "../settings.h"
#include "../game.h"
#include "pxt.h"
#include "org.h"
#include "ogg.h"
#include "ogg11.h"
#include "sound.h"
#include "../common/stat.h"
#include "../ResourceManager.h"

#define MUSIC_OFF		0
#define MUSIC_ON		1
#define MUSIC_BOSS_ONLY	2
static int lastsong = 0;		// this holds the previous song, for <RMU
static int cursong = 0;
int lastsongpos = 0;
bool songlooped = false;

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

static const char *org_dir = "org/";
static const char *ogg_dir = "Ogg/";
static const char *ogg11_dir = "Ogg11/";

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

#if SDL_MIXER_PATCHLEVEL >= 2
	if (Mix_OpenAudioDevice(SAMPLE_RATE, AUDIO_S16, 2, 2048, NULL, 0) == -1)
	{
		staterr("Unable to init mixer.");
		return 1;
	}
#else
	if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16, 2, 2048) == -1)
	{
		staterr("Unable to init mixer.");
		return 1;
	}
#endif
	Mix_AllocateChannels(64);
	
	Mix_ChannelFinished(pxtSoundDone);
	
	if (pxt_init()) return 1;
	
	if (pxt_LoadSoundFX(ResourceManager::getInstance()->getPathForDir("pxt/"), sndcache, NUM_SOUNDS)) return 1;
	
	if (org_init(ResourceManager::getInstance()->getLocalizedPath("wavetable.dat"), ResourceManager::getInstance()->getPathForDir("pxt/"), ORG_VOLUME))
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

static void start_track(int songno, bool resume)
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
	
	if (!org_load(ResourceManager::getInstance()->getLocalizedPath(fname)))
	{
		org_start(resume ? music_lastsongpos() : 0);
	}
}

static void start_ogg_track(int songno, bool resume)
{
	char fname[MAXPATHLEN];

	if (songno == 0)
	{
		ogg_stop();
		return;
	}
	
	strcpy(fname, ogg_dir);
	strcat(fname, org_names[songno]);
	strcat(fname, ".ogg");

	ogg_start(ResourceManager::getInstance()->getLocalizedPath(fname), resume ? music_lastsongpos() : 0);
}

static void start_ogg11_track(int songno, bool resume)
{
	char fname_intro[MAXPATHLEN];
	char fname_loop[MAXPATHLEN];

	if (songno == 0)
	{
		ogg11_stop();
		return;
	}
	
	// There's no intro/loop for lastbtl
	if (strstr(org_names[songno], "lastbtl"))
	{
		strcpy(fname_intro, ogg11_dir);
		strcat(fname_intro, org_names[songno]);
		strcat(fname_intro, ".ogg");

		strcpy(fname_loop, ogg11_dir);
		strcat(fname_loop, org_names[songno]);
		strcat(fname_loop, ".ogg");
	}
	else
	{
		strcpy(fname_intro, ogg11_dir);
		strcat(fname_intro, org_names[songno]);
		strcat(fname_intro, "_intro.ogg");

		strcpy(fname_loop, ogg11_dir);
		strcat(fname_loop, org_names[songno]);
		strcat(fname_loop, "_loop.ogg");
	}

	ogg11_start(ResourceManager::getInstance()->getLocalizedPath(fname_intro), ResourceManager::getInstance()->getLocalizedPath(fname_loop), resume ? music_lastsongpos() : 0, resume ? songlooped : false);
}


void music(int songno, bool resume)
{
	if (songno == cursong)
		return;
	
	lastsong = cursong;
	cursong = songno;
	
	stat(" >> music(%d)", songno);
	
	if (songno != 0 && !should_music_play(songno, settings->music_enabled))
	{
		stat("Not playing track %d because music_enabled is %d", songno, settings->music_enabled);
		switch (settings->new_music)
		{
			case 0:
				org_stop();
				break;
			case 1:
				ogg_stop();
				break;
			case 2:
				ogg11_stop();
				break;
		}
		return;
	}
	
	switch (settings->new_music)
	{
		case 0:
			start_track(songno,resume);
			break;
		case 1:
			start_ogg_track(songno,resume);
			break;
		case 2:
			start_ogg11_track(songno,resume);
			break;
	}
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
		
		switch (settings->new_music)
		{
			case 0:
				if (play != org_is_playing())
				{
					if (play)
						start_track(cursong,0);
					else
						org_stop();
				}
				break;
			case 1:
				if (play != ogg_is_playing())
				{
					if (play)
						start_ogg_track(cursong,0);
					else
						ogg_stop();
				}
				break;
			case 2:
				if (play != ogg11_is_playing())
				{
					if (play)
						start_ogg11_track(cursong,0);
					else
						ogg11_stop();
				}
				break;
		}
	}
}

void music_set_newmusic(int newstate)
{
	if (newstate != settings->new_music)
	{
		stat("music_set_newmusic(%d)", newstate);
		
		settings->new_music = newstate;
		
		org_stop();
		ogg_stop();
		ogg11_stop();
		
		switch (newstate)
		{
			case 0:
				start_track(cursong,0);
				break;
			case 1:
				start_ogg_track(cursong,0);
				break;
			case 2:
				start_ogg11_track(cursong,0);
				break;
		}
	}
}

void music_fade()
{
	switch (settings->new_music)
	{
		case 0:
			org_fade();
			break;
		case 1:
			ogg_fade();
			break;
		case 2:
			ogg11_fade();
			break;
	}
}

int music_cursong()		{ return cursong; }
int music_lastsong() 	{ return lastsong; }
int music_lastsongpos() 	{ return lastsongpos; }

void music_run_fade()
{
	switch (settings->new_music)
	{
		case 0:
			org_run_fade();
			break;
		case 1:
			ogg_run_fade();
			break;
		case 2:
			ogg11_run_fade();
			break;
	}
}

