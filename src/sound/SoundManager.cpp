#include "SoundManager.h"

#include "../ResourceManager.h"
#include "../common/json.hpp"
#include "../common/misc.h"
#include "../common/stat.h"
#include "../game.h"
#include "../settings.h"
#include "Ogg.h"
#include "Organya.h"
#include "Pixtone.h"

#include <fstream>
#include <iostream>

#define MUSIC_OFF 0
#define MUSIC_ON 1
#define MUSIC_BOSS_ONLY 2

namespace NXE
{
namespace Sound
{

SoundManager::SoundManager() {}
SoundManager::~SoundManager() {}

SoundManager *SoundManager::getInstance()
{
  return Singleton<SoundManager>::get();
}

bool SoundManager::init()
{
  if (Mix_Init(MIX_INIT_OGG) == -1)
  {
    staterr("Unable to init mixer.");
    return false;
  }

#if SDL_MIXER_PATCHLEVEL >= 2
  if (Mix_OpenAudioDevice(SAMPLE_RATE, AUDIO_S16, 2, 2048, NULL, 0) == -1)
  {
    staterr("Unable to init mixer.");
    return false;
  }
#else
  if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16, 2, 2048) == -1)
  {
    staterr("Unable to init mixer.");
    return false;
  }
#endif
  Mix_AllocateChannels(64);

  std::string path = ResourceManager::getInstance()->getLocalizedPath("music.json");

  std::ifstream fl;

  _org_names.clear();

  fl.open(widen(path), std::ifstream::in | std::ifstream::binary);
  if (fl.is_open())
  {
    nlohmann::json tracklist = nlohmann::json::parse(fl);

    for (auto it = tracklist.begin(); it != tracklist.end(); ++it)
    {
      _org_names.push_back(it.value());
    }
  }
  else
  {
    staterr("Failed to load tracklist");
    return false;
  }

  Pixtone::getInstance()->init();
  Organya::getInstance()->init();
  return true;
}

void SoundManager::shutdown()
{
  Organya::getInstance()->shutdown();
  Pixtone::getInstance()->shutdown();

  Mix_CloseAudio();
  Mix_Quit();
}

void SoundManager::playSfx(NXE::Sound::SFX snd, int32_t loop)
{
  if (!settings->sound_enabled)
    return;

  Pixtone::getInstance()->stop((int32_t)snd);
  Pixtone::getInstance()->play(-1, (int32_t)snd, loop);
}

void SoundManager::playSfxResampled(NXE::Sound::SFX snd, uint32_t percent)
{
  if (!settings->sound_enabled)
    return;

  Pixtone::getInstance()->playResampled(-1, (int32_t)snd, -1, percent);
}

void SoundManager::stopSfx(NXE::Sound::SFX snd)
{
  Pixtone::getInstance()->stop((int32_t)snd);
}

void SoundManager::startStreamSound(int32_t freq)
{
  playSfxResampled(SFX::SND_STREAM1, freq);
  playSfxResampled(SFX::SND_STREAM2, freq + 100);
}

void SoundManager::startPropSound()
{
  playSfx(SFX::SND_PROPELLOR, -1);
}

void SoundManager::stopLoopSfx()
{
  stopSfx(SFX::SND_STREAM1);
  stopSfx(SFX::SND_STREAM2);
  stopSfx(SFX::SND_PROPELLOR);
}

void SoundManager::music(uint32_t songno, bool resume)
{
  if (songno == _currentSong)
    return;

  _lastSong    = _currentSong;
  _currentSong = songno;

  stat(" >> music(%d)", songno);

  if (songno != 0 && !_shouldMusicPlay(songno, settings->music_enabled))
  {
    stat("Not playing track %d because music_enabled is %d", songno, settings->music_enabled);
    switch (settings->new_music)
    {
      case 0:
        _lastSongPos = Organya::getInstance()->stop();
        break;
      case 1:
      case 2:
        _songlooped  = Ogg::getInstance()->looped();
        _lastSongPos = Ogg::getInstance()->stop();
        break;
    }
    return;
  }

  switch (settings->new_music)
  {
    case 0:
      _start_org_track(songno, resume);
      break;
    case 1:
      _start_ogg_track(songno, resume, _ogg_dir);
      break;
    case 2:
      _start_ogg_track(songno, resume, _ogg11_dir);
      break;
  }
}

void SoundManager::enableMusic(int newstate)
{
  if (newstate != settings->music_enabled)
  {
    stat("music_set_enabled(%d)", newstate);

    settings->music_enabled = newstate;
    bool play               = _shouldMusicPlay(_currentSong, newstate);

    switch (settings->new_music)
    {
      case 0:
        if (play != Organya::getInstance()->isPlaying())
        {
          if (play)
            _start_org_track(_currentSong, 0);
          else
            _lastSongPos = Organya::getInstance()->stop();
        }
        break;
      case 1:
        if (play != Ogg::getInstance()->isPlaying())
        {
          if (play)
            _start_ogg_track(_currentSong, 0, _ogg_dir);
          else
            _lastSongPos = Ogg::getInstance()->stop();
        }
        break;
      case 2:
        if (play != Ogg::getInstance()->isPlaying())
        {
          if (play)
            _start_ogg_track(_currentSong, 0, _ogg11_dir);
          else
            _lastSongPos = Ogg::getInstance()->stop();
        }
        break;
    }
  }
}

void SoundManager::setNewmusic(int newstate)
{
  if (newstate != settings->new_music)
  {
    stat("music_set_newmusic(%d)", newstate);

    settings->new_music = newstate;

    Organya::getInstance()->stop();
    Ogg::getInstance()->stop();

    switch (newstate)
    {
      case 0:
        _start_org_track(_currentSong, 0);
        break;
      case 1:
        _start_ogg_track(_currentSong, 0, _ogg_dir);
        break;
      case 2:
        _start_ogg_track(_currentSong, 0, _ogg11_dir);
        break;
    }
  }
}

uint32_t SoundManager::currentSong()
{
  return _currentSong;
}

uint32_t SoundManager::lastSong()
{
  return _lastSong;
}

void SoundManager::fadeMusic()
{
  switch (settings->new_music)
  {
    case 0:
      Organya::getInstance()->fade();
      break;
    case 1:
    case 2:
      Ogg::getInstance()->fade();
      break;
  }
}

void SoundManager::runFade()
{
  switch (settings->new_music)
  {
    case 0:
      Organya::getInstance()->runFade();
      break;
    case 1:
    case 2:
      Ogg::getInstance()->runFade();
      break;
  }
}

void SoundManager::pause()
{
  Mix_Pause(-1);
  switch (settings->new_music)
  {
    case 0:
      Organya::getInstance()->pause();
      break;
    case 1:
    case 2:
      Ogg::getInstance()->pause();
      break;
  }
}

void SoundManager::resume()
{
  Mix_Resume(-1);
  switch (settings->new_music)
  {
    case 0:
      Organya::getInstance()->resume();
      break;
    case 1:
    case 2:
      Ogg::getInstance()->resume();
      break;
  }
}

void SoundManager::updateSfxVolume()
{
  Mix_Volume(-1, (int)(128. / 100. * (double)settings->sfx_volume));
}

void SoundManager::updateMusicVolume()
{
  Ogg::getInstance()->updateVolume();
}

bool SoundManager::_shouldMusicPlay(uint32_t songno, uint32_t musicmode)
{
  if (game.mode == GM_TITLE || game.mode == GM_CREDITS)
    return true;

  switch (musicmode)
  {
    case MUSIC_OFF:
      return false;
    case MUSIC_ON:
      return true;
    case MUSIC_BOSS_ONLY:
      return _musicIsBoss(songno);
  }

  return false;
}

bool SoundManager::_musicIsBoss(uint32_t songno)
{
  if (strchr(_bossmusic, songno))
    return true;
  else
    return false;
}

void SoundManager::_start_org_track(int songno, bool resume)
{
  _lastSongPos = Organya::getInstance()->stop();
  if (songno == 0)
  {
    return;
  }

  if (Organya::getInstance()->load(
          ResourceManager::getInstance()->getLocalizedPath(_org_dir + _org_names[songno] + ".org")))
  {
    Organya::getInstance()->start(resume ? _lastSongPos : 0);
  }
}

void SoundManager::_start_ogg_track(int songno, bool resume, std::string dir)
{
  if (songno == 0)
  {
    _songlooped  = Ogg::getInstance()->looped();
    _lastSongPos = Ogg::getInstance()->stop();
    return;
  }
  Ogg::getInstance()->start(_org_names[songno], dir, resume ? _lastSongPos : 0, resume ? _songlooped : false);
}

} // namespace Sound
} // namespace NXE
