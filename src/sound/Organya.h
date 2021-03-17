/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#ifndef _ORGANYA_H
#define _ORGANYA_H

#include "../Singleton.h"
#include "SoundManager.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace NXE
{
namespace Sound
{

struct Instrument
{
  int32_t tuning, wave, wave_step, last_note;
  bool playing;
  bool pipi;
  std::size_t n_events;

  struct Event
  {
    int32_t note, length, volume, panning;
  };
  std::map<int32_t /*beat*/, Event> events;

  // Volatile data, used & changed during playback:
  double phaseacc, phaseinc, cur_vol;
  int32_t cur_pan, cur_length, cur_wavesize;
  const int16_t *cur_wave;
};

struct Song
{
  uint32_t ms_per_beat, samples_per_beat, loop_start, loop_end;
  uint32_t steps_per_bar, beats_per_step;
  Instrument ins[16];
  bool loaded = false;
  bool playing = false;
  uint32_t cur_beat    = 0;
  uint32_t total_beats = 0;
  uint32_t last_pos    = 0;
  std::vector<float> samples;
  uint32_t volume = 255;
  bool Load(const std::string &fname);
  void Synth();
  uint32_t last_gen_tick;
  uint32_t last_gen_beat;
};

class Organya
{

public:
  static Organya *getInstance();

  void init();
  void shutdown();

  bool load(const std::string &fname);
  bool start(int startBeat);
  uint32_t stop();
  bool isPlaying();
  void fade();
  void setVolume(float newVolume);
  void runFade();
  void pause();
  void resume();
  void _musicCallback(void *udata, uint8_t *stream, uint32_t len);

protected:
  friend class Singleton<Organya>;

  Organya();
  ~Organya();
  Organya(const Organya &) = delete;
  Organya &operator=(const Organya &) = delete;

private:
  void _setPlayPosition(uint32_t pos);
  bool _loadWavetable();
  bool _loadDrums();

  Song song;
  const SFX drum_pxt_table[8] = {
    SFX::SND_DRUM_BASS, 
    SFX::SND_DRUM_SNARE, 
    SFX::SND_DRUM_HICLOSE, 
    SFX::SND_DRUM_HIOPEN, 
    SFX::SND_DRUM_TOM, 
    SFX::SND_DRUM_PERCUSSION,
    SFX::SND_NULL,
    SFX::SND_NULL,
  };
  float volume            = 1.0;
  bool fading             = false;
  uint32_t last_fade_time = 0;
};

} // namespace Sound
} // namespace NXE

#endif
