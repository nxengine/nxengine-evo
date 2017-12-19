
#ifndef _OGG_H
#define _OGG_H

struct oggSong
{
	Mix_Music *music;
	bool playing;
	int volume;
	
	bool fading;
	uint32_t last_fade_time;
	uint32_t last_pos;
};

char ogg_load(const std::string& fname);
bool ogg_start(const std::string& fname, int startbeat);
void ogg_stop(void);
bool ogg_is_playing(void);
void ogg_fade(void);
void ogg_set_volume(int newvolume);
void ogg_run_fade(void);

#endif

