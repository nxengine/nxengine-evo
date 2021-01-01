/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#include "Organya.h"

#include "../ResourceManager.h"
#include "../common/glob.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../Utils/Common.h"
#include "../settings.h"
#include "Pixtone.h"
#include "SoundManager.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

//------------------------------------------------------------------------------

#define MIN_VOLUME 0.204

namespace NXE
{
namespace Sound
{

static int16_t WaveTable[100 * 256] = {0};
static std::vector<int16_t> DrumSamples[8];

Organya::Organya()
{
  LOG_INFO("Organya init...");
  _loadWavetable();
  _loadDrums();
  LOG_INFO("Organya init done");
}

Organya::~Organya() {}

Organya *Organya::getInstance()
{
  return Singleton<Organya>::get();
}

void Organya::init() {}

void Organya::shutdown() {}

//------------------------------------------------------------------------------

bool Organya::_loadWavetable()
{
  FILE *fp
      = myfopen(widen(ResourceManager::getInstance()->getLocalizedPath("wavetable.dat")).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_ERROR("Unable to open wavetable.dat");
    return false;
  }

  for (size_t a = 0; a < 100 * 256; ++a)
    WaveTable[a] = (signed char)fgetc(fp);

  std::fclose(fp);

  return true;
}

bool Organya::_loadDrums()
{
  for (uint8_t drumno = 0; drumno < 8; ++drumno)
  {
    if (drum_pxt_table[drumno] == SFX::SND_NULL)
      continue; // Leave that non-existed drum file unloaded

    // Load the drum parameters
    char fname[256];
    sprintf(fname, "%sfx%02x.pxt", ResourceManager::getInstance()->getPathForDir("pxt/").c_str(), (uint8_t) drum_pxt_table[drumno]);

    stPXSound snd;

    LOG_DEBUG("load_drum: loading {} into drum index {}", fname, drumno);

    if (!snd.load(fname))
      return false;

    snd.render();

    // Synthesize and mix the drum's component channels
    auto &sample = DrumSamples[drumno];

    if (snd.final_size > sample.size())
      sample.resize(snd.final_size, 0);
    for (size_t a = 0; a < snd.final_size; ++a)
      sample[a] = snd.final_buffer[a];

    snd.freeBuf();
  }

  return true;
}

void Organya::_setPlayPosition(uint32_t pos)
{
  song.cur_beat = pos;
}

bool Song::Load(const std::string &fname)
{
  FILE *fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_WARN("Song::Load: no such file: '{}'", fname);
    return false;
  }
  for (int i = 0; i < 6; ++i)
    fgetc(fp); // Ignore file signature ("Org-02")
  last_pos    = 0;
  cur_beat    = 0;
  total_beats = 0;
  // Load song parameters
  ms_per_beat    = fgeti(fp);
  steps_per_bar  = fgetc(fp);
  beats_per_step = fgetc(fp);
  loop_start     = fgetl(fp);
  loop_end       = fgetl(fp);
  // Load each instrument parameters (and initialize them)
  for (auto &i : ins)
  {
    i = {fgeti(fp), fgetc(fp), fgetc(fp) != 0, fgeti(fp), {}, 0, 0, 0, 0, 0, 0, 0};
  }
  // Load events for each instrument
  for (auto &i : ins)
  {
    std::vector<std::pair<int, Instrument::Event>> events(i.n_events);
    for (auto &n : events)
      n.first = fgetl(fp);
    for (auto &n : events)
      n.second.note = fgetc(fp);
    for (auto &n : events)
      n.second.length = fgetc(fp);
    for (auto &n : events)
      n.second.volume = fgetc(fp);
    for (auto &n : events)
      n.second.panning = fgetc(fp);
    i.events.insert(events.begin(), events.end());
  }
  std::fclose(fp);
  return true;
}

void Song::Synth()
{
  unsigned sampling_rate = SAMPLE_RATE;
  // Determine playback settings:
  double samples_per_millisecond = sampling_rate * 1e-3, master_volume = 4e-6;
  int samples_per_beat = ms_per_beat * samples_per_millisecond; // rounded.

  // Begin synthesis

  last_gen_tick = SDL_GetTicks();
  last_gen_beat = cur_beat;

  if (cur_beat == loop_end)
    cur_beat = loop_start;
  // Synthesize this beat in stereo sound (two channels).
  samples.clear();
  samples.resize(0);
  samples.resize(samples_per_beat * 2, 0.f);

  for (auto &i : ins)
  {
    // Check if there is an event for this beat
    auto j = i.events.find(cur_beat);
    if (j != i.events.end())
    {
      auto &event = j->second;
      if (event.volume != 255)
        i.cur_vol = event.volume * master_volume;
      if (event.panning != 255)
        i.cur_pan = event.panning;
      if (event.note != 255)
      {
        // Calculate the note's wave data sampling frequency (equal temperament)
        double freq = std::pow(2.0, (event.note + i.tuning / 1000.0 + 155.376) / 12);
        // Note: 155.376 comes from:
        //         12*log(256*440)/log(2) - (4*12-3-1) So that note 4*12-3 plays at 440 Hz.
        // Note: Optimizes into
        //         pow(2, (note+155.376 + tuning/1000.0) / 12.0)
        //         2^(155.376/12) * exp( (note + tuning/1000.0)*log(2)/12 )
        // i.e.    7901.988*exp(0.057762265*(note + tuning*1e-3))
        i.phaseinc = freq / sampling_rate;
        i.phaseacc = 0;
        // And determine the actual wave data to play
        i.cur_wave     = &WaveTable[256 * (i.wave % 100)];
        i.cur_wavesize = 256;
        i.cur_length   = i.pipi ? 1024 / i.phaseinc : (event.length * samples_per_beat);
        if (&i >= &ins[8]) // Percussion is different
        {
          const auto idx = &i - &ins[0];
          const auto &d  = DrumSamples[idx - 8];
          i.phaseinc     = event.note * (22050 / 32.5) / sampling_rate; // Linear frequency
          i.cur_wave     = &d[0];
          i.cur_wavesize = d.size();
          i.cur_length   = d.size() / i.phaseinc;
        }
        // Ignore missing drum samples
        if (i.cur_wavesize <= 0)
        {
          i.cur_length = 0;
        }
      }
    }

    // Generate wave data. Calculate left & right volumes...
    static const int panning_table[13] = {0, 43, 86, 129, 172, 215, 256, 297, 340, 383, 426, 469, 512};
    const double pan = (panning_table[clamp(i.cur_pan, 0, 12)] - 256) * 10;
    double left = 1.0;
    double right = 1.0;

    if (pan < 0) {
      right = pow(10.0, ((double) pan) / 2000.0);
    } else if (pan > 0) {
      left = pow(10.0, ((double) -pan) / 2000.0);
    }

    left *= i.cur_vol * 8;
    right *= i.cur_vol * 8;

    int n = samples_per_beat > i.cur_length ? i.cur_length : samples_per_beat;
    for (int p = 0; p < n; ++p)
    {
      const double pos = i.phaseacc;
      // Take a sample from the wave data.
      double sample = 0;

#ifndef _LOW_END_HARDWARE

#ifdef _LINEAR_INTERPOLATION
      // Perform linear interpolation
      unsigned int position_integral = unsigned(pos);
      const double position_fractional = pos - position_integral;

      const double sample1 = i.cur_wave[position_integral % i.cur_wavesize];
      const double sample2 = i.cur_wave[(position_integral + 1) % i.cur_wavesize];

      sample = sample1 + (sample2 - sample1) * position_fractional;
#else
      // Perform cubic interpolation
      const unsigned int position_integral = unsigned(pos) % i.cur_wavesize;
      const double position_fractional = pos - (double)((int) pos);
      const float s1 = (float) (i.cur_wave[position_integral]);
      const float s2 = (float) (i.cur_wave[clamp(position_integral + 1, (unsigned int) 0, (unsigned int) i.cur_wavesize - 1)]);
      const float sp = (float) (i.cur_wave[clamp(position_integral + 2, (unsigned int) 0, (unsigned int) i.cur_wavesize - 1)]);
      const float sn = (float) (i.cur_wave[MAX((int) position_integral - 1, 0)]);
      const float mu2 = position_fractional * position_fractional;
      const float a0 = sn - s2 - sp + s1;
      const float a1 = sp - s1 - a0;
      const float a2 = s2 - sp;
      const float a3 = s1;
      sample = a0 * position_fractional * mu2 + a1 * mu2 + a2 * position_fractional + a3;
#endif

#else
      // Perform nearest-neighbour interpolation
      sample = i.cur_wave[ unsigned(pos) % i.cur_wavesize ];
#endif

      // Save audio in float32 format:
      samples[p * 2 + 0] += sample * left;
      samples[p * 2 + 1] += sample * right;
      i.phaseacc += i.phaseinc;
    }
    i.cur_length -= n;
  }
  ++cur_beat;
}

bool Organya::load(const std::string &fname)
{
  song.loaded  = false;
  song.playing = false;

  if (!song.Load(fname))
    return false;
  // Reset position
  _setPlayPosition(0);

  // Set as loaded
  song.loaded = true;

  return true;
}

std::function<void(void *, uint8_t *, int)> musicCallback;

void myMusicPlayer(void *udata, uint8_t *stream, int len)
{
  musicCallback(udata, stream, len);
}

void Organya::_musicCallback(void *udata, uint8_t *stream, uint32_t len)
{
  SDL_memset(stream, 0, len);
  if (!song.playing)
    return;
  int16_t *str = reinterpret_cast<int16_t *>(stream);

  uint32_t idx = song.last_pos;
  for (uint32_t i = 0; i < len / 2; i++)
  {
    if (idx >= song.samples.size())
    {
      song.Synth();
      idx = 0;
    }
    // extended range
    int32_t sample = song.samples[idx] * 32767.0 * volume * (double)(settings->music_volume / 100.);
    // clip to int16
    if (sample > 32767)
    {
      sample = 32767;
    }
    if (sample < -32768)
    {
      sample = -32768;
    }

    str[i] = (int16_t)sample;
    idx++;
  }

  song.last_pos = idx;
}

bool Organya::start(int startBeat)
{
  if (!song.loaded)
    return false;

  song.playing = true;
  fading       = false;
  volume       = 0.75;
  musicCallback
      = std::bind(&Organya::_musicCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  _setPlayPosition(startBeat);
  song.Synth();
  Mix_HookMusic(myMusicPlayer, NULL);
  return true;
}

uint32_t Organya::stop()
{
  if (song.playing)
  {
    /* Okay, this is hackish, and still misses a beat or two.
       Sadly, there's no better way on SDL, because it writes
       to audio device in bulk and there's no way of knowing
       how many samples actually played.
    */
    uint32_t delta    = SDL_GetTicks() - song.last_gen_tick;
    uint32_t beats    = (double)delta / (double)song.ms_per_beat;
    uint32_t cur_beat = song.last_gen_beat + beats;
    if (cur_beat >= song.loop_end)
    {
      cur_beat = song.loop_start + (cur_beat - song.loop_end);
    }

    song.playing = false;
    Mix_HookMusic(NULL, NULL);
    return cur_beat;
  }
  return 0;
}

bool Organya::isPlaying()
{
  return song.playing;
}

void Organya::fade()
{
  fading         = true;
  last_fade_time = 0;
}

void Organya::pause()
{
  song.playing = false;
}

void Organya::resume()
{
  song.playing = true;
}

void Organya::setVolume(float newVolume)
{
  volume = newVolume;
}

void Organya::runFade()
{
  if (!fading)
    return;
  uint32_t curtime = SDL_GetTicks();

  if ((curtime - last_fade_time) >= 25)
  {
    float newvol = (volume - 0.01);
    if (newvol <= 0.0)
    {
      fading = false;
      stop();
    }
    else
    {
      setVolume(newvol);
    }
    last_fade_time = curtime;
  }
}

} // namespace Sound
} // namespace NXE
