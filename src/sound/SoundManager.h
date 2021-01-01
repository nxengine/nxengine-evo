#ifndef __SOUNDMANAGER_H_
#define __SOUNDMANAGER_H_

#include "../Singleton.h"

#include <SDL_mixer.h>
#include <cstdint>
#include <string>
#include <vector>

#define SAMPLE_RATE 44100

namespace NXE
{
namespace Sound
{

enum class SFX : uint16_t
{
  SND_NULL             = 0,
  SND_MENU_MOVE        = 1,
  SND_MSG              = 2,
  SND_BONK_HEAD        = 3,
  SND_SWITCH_WEAPON    = 4,
  SND_MENU_PROMPT      = 5,
  SND_HOPPY_JUMP       = 6,
  SND_DOOR             = 11,
  SND_BLOCK_DESTROY    = 12,
  SND_GET_XP           = 14,
  SND_PLAYER_JUMP      = 15,
  SND_PLAYER_HURT      = 16,
  SND_PLAYER_DIE       = 17,
  SND_MENU_SELECT      = 18,
  SND_HEALTH_REFILL    = 20,
  SND_BUBBLE           = 21,
  SND_CHEST_OPEN       = 22,
  SND_THUD             = 23,
  SND_PLAYER_WALK      = 24,
  SND_FUNNY_EXPLODE    = 25,
  SND_QUAKE            = 26,
  SND_LEVEL_UP         = 27,
  SND_SHOT_HIT         = 28,
  SND_TELEPORT         = 29,
  SND_ENEMY_JUMP       = 30,
  SND_TINK             = 31,
  SND_POLAR_STAR_L1_2  = 32,
  SND_SNAKE_FIRE       = 33,
  SND_FIREBALL         = 34,
  SND_EXPLOSION1       = 35,
  SND_GUN_CLICK        = 37,
  SND_GET_ITEM         = 38,
  SND_EM_FIRE          = 39,
  SND_STREAM1          = 40,
  SND_STREAM2          = 41,
  SND_GET_MISSILE      = 42,
  SND_COMPUTER_BEEP    = 43,
  SND_MISSILE_HIT      = 44,
  SND_XP_BOUNCE        = 45,
  SND_IRONH_SHOT_FLY   = 46,
  SND_EXPLOSION2       = 47,
  SND_BUBBLER_FIRE     = 48,
  SND_POLAR_STAR_L3    = 49,
  SND_ENEMY_SQUEAK     = 50,
  SND_ENEMY_HURT       = 51,
  SND_ENEMY_HURT_BIG   = 52,
  SND_ENEMY_HURT_SMALL = 53,
  SND_ENEMY_HURT_COOL  = 54,
  SND_ENEMY_SQUEAK2    = 55,
  SND_SPLASH           = 56,
  SND_ENEMY_DAMAGE     = 57,
  SND_PROPELLOR        = 58,
  SND_SPUR_CHARGE_1    = 59,
  SND_SPUR_CHARGE_2    = 60,
  SND_SPUR_CHARGE_3    = 61,
  SND_SPUR_FIRE_1      = 62,
  SND_SPUR_FIRE_2      = 63,
  SND_SPUR_FIRE_3      = 64,
  SND_SPUR_MAXED       = 65,
  SND_EXPL_SMALL       = 70,
  SND_LITTLE_CRASH     = 71,
  SND_BIG_CRASH        = 72,
  SND_BUBBLER_LAUNCH   = 100,
  SND_LIGHTNING_STRIKE = 101,
  SND_JAWS             = 102,
  SND_CHARGE_GUN       = 103,
  SND_104              = 104,
  SND_PUPPY_BARK       = 105,
  SND_SLASH            = 106,
  SND_BLOCK_MOVE       = 107,
  SND_IGOR_JUMP        = 108,
  SND_CRITTER_FLY      = 109,
  SND_DROLL_SHOT_FLY   = 110,
  SND_MOTOR_RUN        = 111,
  SND_MOTOR_SKIP       = 112,
  SND_BOOSTER          = 113,
  SND_CORE_HURT        = 114,
  SND_CORE_THRUST      = 115,
  SND_CORE_CHARGE      = 116,
  SND_NEMESIS_FIRE     = 117,
  // drums
  SND_DRUM_BASS = 150,
  SND_DRUM_SNARE = 151,
  SND_DRUM_HICLOSE = 152,
  SND_DRUM_HIOPEN = 153,
  SND_DRUM_TOM = 154,
  SND_DRUM_PERCUSSION = 155
};

class SoundManager
{

public:
  static SoundManager *getInstance();

  bool init();
  void shutdown();

  void playSfx(NXE::Sound::SFX snd, int32_t loop = 0);
  void playSfxResampled(NXE::Sound::SFX snd, uint32_t percent);
  void stopSfx(NXE::Sound::SFX snd);

  void startStreamSound(int32_t freq);
  void startPropSound();
  void stopLoopSfx();

  void music(uint32_t songno, bool resume = false);
  void enableMusic(int newstate);
  void setNewmusic(int newstate);

  uint32_t currentSong();
  uint32_t lastSong();
  void fadeMusic();
  void runFade();
  void pause();
  void resume();
  void updateMusicVolume();
  void updateSfxVolume();
  std::vector<std::string> &music_dir_names();

protected:
  friend class Singleton<SoundManager>;

  SoundManager();
  ~SoundManager();
  SoundManager(const SoundManager &) = delete;
  SoundManager &operator=(const SoundManager &) = delete;

private:
  bool _shouldMusicPlay(uint32_t songno, uint32_t musicmode);
  bool _musicIsBoss(uint32_t songno);
  void _start_org_track(int songno, bool resume);
  void _start_ogg_track(int songno, bool resume, std::string dir);
  void _reloadTrackList();

  uint32_t _lastSong    = 0;
  uint32_t _lastSongPos = 0;
  uint32_t _currentSong = 0;
  bool _songlooped      = false;

  std::vector<std::string> _music_names;
  std::vector<bool> _music_loop;
  std::vector<std::string> _music_dirs;
  std::vector<std::string> _music_dir_names;
  std::vector<std::string> _music_playlists;

  const char _bossmusic[14] = {4, 7, 10, 11, 15, 16, 17, 18, 21, 22, 31, 33, 35, 0};
};



} // namespace Sound
} // namespace NXE
#endif
