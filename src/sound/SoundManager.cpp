#include "SoundManager.h"

#include "../ResourceManager.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../game.h"
#include "../settings.h"
#include "Ogg.h"
#include "Organya.h"
#include "Pixtone.h"

#include <json.hpp>
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
  LOG_INFO("Sound system init");
  if (Mix_Init(MIX_INIT_OGG) == -1)
  {
    LOG_ERROR("Unable to init mixer.");
    return false;
  }

#if SDL_MIXER_PATCHLEVEL >= 2
  if (Mix_OpenAudioDevice(SAMPLE_RATE, AUDIO_S16, 2, 4096, NULL, 0) == -1)
  {
    LOG_ERROR("Unable to open audio device.");
    return false;
  }
#else
  if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16, 2, 2048) == -1)
  {
    LOG_ERROR("Unable to open audio device.");
    return false;
  }
#endif
  Mix_AllocateChannels(64);

  std::string path = ResourceManager::getInstance()->getPath("music_dirs.json", false);

  _music_dirs.clear();
  _music_dir_names.clear();
  _music_playlists.clear();
  _music_dirs.push_back("org/");
  _music_dir_names.push_back("Original");
  _music_playlists.push_back("music.json");

  std::ifstream fl;

  fl.open(widen(path), std::ifstream::in | std::ifstream::binary);
  if (fl.is_open())
  {
    nlohmann::json dirlist = nlohmann::json::parse(fl);

    for (auto it = dirlist.begin(); it != dirlist.end(); ++it)
    {
      std::string dir = it.value().at("dir");
      if (
        ResourceManager::getInstance()->fileExists(
          ResourceManager::getInstance()->getPathForDir(dir)
        )
      )
      {
        auto it_playlist = it.value().find("playlist");
        if (it_playlist != it.value().end())
        {
          _music_playlists.push_back(*it_playlist);
        }
        else
        {
          _music_playlists.push_back("music.json");
        }

        _music_dirs.push_back(dir);
        _music_dir_names.push_back(it.value().at("name"));
      }
      else
      {
        LOG_WARN("Music dir {} doesn't exist", dir.c_str());
      }
    }
    fl.close();
  }
  else
  {
    LOG_ERROR("Failed to load music_dirs.json");
  }

  _reloadTrackList();

  Pixtone::getInstance()->init();
  Organya::getInstance()->init();

  // prepare resampled stream sounds (Core battle and <SSS in main artery)
  Pixtone::getInstance()->prepareResampled((int32_t)SFX::SND_STREAM1, 1000);
  Pixtone::getInstance()->prepareResampled((int32_t)SFX::SND_STREAM2, 1100);
  Pixtone::getInstance()->prepareResampled((int32_t)SFX::SND_STREAM1, 400);
  Pixtone::getInstance()->prepareResampled((int32_t)SFX::SND_STREAM2, 500);

  updateSfxVolume();
  updateMusicVolume();
  return true;
}

void SoundManager::shutdown()
{
  Organya::getInstance()->shutdown();
  Pixtone::getInstance()->shutdown();

  Mix_CloseAudio();
  Mix_Quit();
  LOG_INFO("Sound system shutdown");
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

  LOG_DEBUG(" >> music({})", songno);

  if (songno != 0 && !_shouldMusicPlay(songno, settings->music_enabled))
  {
    LOG_INFO("Not playing track {} because music_enabled is {}", songno, settings->music_enabled);
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
    default:
      _start_ogg_track(songno, resume, _music_dirs.at(settings->new_music));
      break;
  }
}

void SoundManager::enableMusic(int newstate)
{
  if (newstate != settings->music_enabled)
  {
    LOG_DEBUG("enableMusic({})", newstate);

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
      default:
        if (play != Ogg::getInstance()->isPlaying())
        {
          if (play)
            _start_ogg_track(_currentSong, 0, _music_dirs.at(settings->new_music));
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
    LOG_DEBUG("setNewMusic({})", newstate);

    settings->new_music = newstate;

    Organya::getInstance()->stop();
    Ogg::getInstance()->stop();

    _reloadTrackList();

    switch (newstate)
    {
      case 0:
        _start_org_track(_currentSong, 0);
        break;
      default:
        _start_ogg_track(_currentSong, 0, _music_dirs.at(newstate));
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
  if (_music_names.size() < 2) return;

  _lastSongPos = Organya::getInstance()->stop();
  if (songno == 0)
  {
    return;
  }

  if (Organya::getInstance()->load(
          ResourceManager::getInstance()->getPath(_music_dirs.at(0) + _music_names[songno] + ".org", false)))
  {
    Organya::getInstance()->start(resume ? _lastSongPos : 0);
  }
}

void SoundManager::_start_ogg_track(int songno, bool resume, std::string dir)
{
  if (_music_names.size() < 2) return;

  if (songno == 0)
  {
    _songlooped  = Ogg::getInstance()->looped();
    _lastSongPos = Ogg::getInstance()->stop();
    return;
  }
  Ogg::getInstance()->start(_music_names[songno], dir, resume ? _lastSongPos : 0, resume ? _songlooped : false, _music_loop[songno]);
}

std::vector<std::string> &SoundManager::music_dir_names()
{
  return _music_dir_names;
}

void SoundManager::_reloadTrackList()
{
  if (_music_playlists.size() <= settings->new_music)
    settings->new_music = 0;

  std::string path = ResourceManager::getInstance()->getPath(_music_playlists.at(settings->new_music), false);

  std::ifstream fl;

  _music_names.clear();
  _music_names.push_back("");
  _music_loop.clear();
  _music_loop.push_back(false);

  fl.open(widen(path), std::ifstream::in | std::ifstream::binary);
  if (fl.is_open())
  {
    nlohmann::json tracklist = nlohmann::json::parse(fl);

    for (auto it = tracklist.begin(); it != tracklist.end(); ++it)
    {
      auto it_loop = it.value().find("loop");
      if (it_loop != it.value().end())
      {
        _music_loop.push_back(*it_loop);
      }
      else
      {
        _music_loop.push_back(true);
      }
      _music_names.push_back(it.value().at("name"));
    }
    fl.close();
  }
  else
  {
    LOG_ERROR("Failed to load music.json");
  }
}

} // namespace Sound
} // namespace NXE
