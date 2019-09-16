#include "Ogg.h"

#include "../ResourceManager.h"
#include "../common/basics.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../settings.h"
#include "SoundManager.h" // SAMPLE_RATE

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

namespace NXE
{
namespace Sound
{

Ogg *Ogg::getInstance()
{
  return Singleton<Ogg>::get();
}

Ogg::Ogg() {}
Ogg::~Ogg() {}

bool Ogg::load(const std::string &fname, const std::string &dir, bool doloop)
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
  _song.intro = NULL;
  _song.intro = Mix_LoadMUS(filename.c_str());
  if (!_song.intro)
  {
    LOG_ERROR("Mix_LoadMUS(): {}", Mix_GetError());
    return false;
  }

  filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + "_loop.ogg");
  if (!ResourceManager::fileExists(filename))
  {
    filename = ResourceManager::getInstance()->getLocalizedPath(dir + fname + ".ogg");
  }

  _song.loop = NULL;
  _song.loop = Mix_LoadMUS(filename.c_str());
  if (!_song.loop)
  {
    LOG_ERROR("Mix_LoadMUS(): {}", Mix_GetError());
    return false;
  }

  _song.doloop = doloop;

  return true;
}

static void musicFinishedCallback()
{
  Ogg::getInstance()->musicFinished();
}

void Ogg::musicFinished()
{
  // Here be dragons
  // SDL_mixer documantation states that we should never call SDL_mixer functions from callback
  // However, using flag/event/whatever to restart music in main loop
  // produces audible pause.
  // This seems to work, but needs extensive testing.

  _looped        = true;
  _song.last_pos = SDL_GetTicks();
  if (_song.doloop)
  {
    Mix_PlayMusic(_song.loop, 0);
  }
}

// start the currently-loaded track playing at beat startbeat.
bool Ogg::start(const std::string &fname, const std::string &dir, int startbeat, bool loop, bool doloop)
{
  stop(); // stop any old music

  if (!load(fname, dir, doloop))
  {
    return false;
    _song.playing = false;
  }

  _song.last_pos = 0;
  _song.playing  = true;
  _song.fading   = false;

  _song.volume = 75;
  _do_loop     = false;

  _looped = loop;

  _song.last_pos = SDL_GetTicks() - startbeat;

  if (_looped)
  {
    Mix_PlayMusic(_song.loop, 0);
  }
  else
  {
    Mix_PlayMusic(_song.intro, 0);
  }
  Mix_VolumeMusic((double)_song.volume * ((double)settings->music_volume / 100.));
  Mix_SetMusicPosition(startbeat / 1000);

  Mix_HookMusicFinished(musicFinishedCallback);
  return true;
}

void Ogg::updateVolume()
{
  Mix_VolumeMusic((double)_song.volume * ((double)settings->music_volume / 100.));
}

// pause/stop playback of the current song
uint32_t Ogg::stop()
{
  if (_song.playing)
  {
    _song.playing = false;
    _do_loop      = false;
    _looped       = false;
    Mix_HookMusicFinished(NULL);
    Mix_HaltMusic();
    if (_song.intro)
    {
      Mix_FreeMusic(_song.intro);
      _song.intro = nullptr;
    }
    if (_song.loop)
    {
      Mix_FreeMusic(_song.loop);
      _song.loop = nullptr;
    }
    return SDL_GetTicks() - _song.last_pos;
  }
  return 0;
}

bool Ogg::isPlaying()
{
  return _song.playing;
}

void Ogg::fade()
{
  _song.fading         = true;
  _song.last_fade_time = 0;
}

void Ogg::setVolume(int newvolume)
{
  if (newvolume != _song.volume)
  {
    _song.volume = newvolume;
    Mix_VolumeMusic((double)_song.volume * ((double)settings->music_volume / 100.));
  }
}

void Ogg::runFade()
{

  if (!_song.fading)
    return;
  uint32_t curtime = SDL_GetTicks();
  if ((curtime - _song.last_fade_time) >= 25)
  {
    int newvol = (_song.volume - 1);
    if (newvol <= 0)
    {
      _song.fading = false;
      stop();
    }
    else
    {
      setVolume(newvol);
    }

    _song.last_fade_time = curtime;
  }
}

void Ogg::pause()
{
  Mix_PauseMusic();
}

void Ogg::resume()
{
  Mix_ResumeMusic();
}

bool Ogg::looped()
{
  return _looped;
}

}; // namespace Sound
}; // namespace NXE