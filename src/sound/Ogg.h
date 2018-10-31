
#ifndef _OGG11_H
#define _OGG11_H

#include "../Singleton.h"

#include <SDL_mixer.h>
#include <string>

typedef void (*music_finished_cb)(void);

struct oggSong
{
  Mix_Music *intro = nullptr;
  Mix_Music *loop  = nullptr;
  bool playing     = false;
  int volume       = 75;

  bool fading             = false;
  uint32_t last_fade_time = 0;
  uint32_t last_pos       = 0;
  bool doloop             = false;
};

class Ogg
{
public:
  static Ogg *getInstance();

protected:
  friend class Singleton<Ogg>;

  Ogg();
  ~Ogg();
  Ogg(const Ogg &) = delete;
  Ogg &operator=(const Ogg &) = delete;

public:
  bool load(const std::string &fname, const std::string &dir);
  bool start(const std::string &fname, const std::string &dir, int startbeat, bool loop);
  uint32_t stop(void);
  bool isPlaying(void);
  void fade(void);
  void setVolume(int newvolume);
  void runFade(void);
  void musicFinished();
  bool looped();

private:
  oggSong _song;
  bool _do_loop = false;
  bool _looped  = false;
};

bool ogg11_load(const std::string &fname, const std::string &dir);
bool ogg11_start(const std::string &fname, const std::string &dir, int startbeat, bool loop);
uint32_t ogg11_stop(void);
bool ogg11_is_playing(void);
void ogg11_fade(void);
void ogg11_set_volume(int newvolume);
void ogg11_run_fade(void);

#endif
