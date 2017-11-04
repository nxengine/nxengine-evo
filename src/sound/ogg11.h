
#ifndef _OGG11_H
#define _OGG11_H

struct ogg11Song
{
	Mix_Music *intro;
	Mix_Music *loop;
	bool playing;
	int volume;
	
	bool fading;
	uint32_t last_fade_time;
	uint32_t last_pos;
};

char ogg11_load(char *fname_intro,char *fname_loop);
bool ogg11_start(char *fname_intro, char *fname_loop, int startbeat, bool loop);
void ogg11_stop(void);
bool ogg11_is_playing(void);
void ogg11_fade(void);
void ogg11_set_volume(int newvolume);
void ogg11_run_fade(void);

#endif

