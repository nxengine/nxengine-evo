/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#include "Pixtone.h"

#include "../ResourceManager.h"
#include "../common/misc.h"
#include "../common/stat.h"
#include "../config.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

// using std::fgetc;

namespace NXE
{
namespace Sound
{

static struct
{
  int8_t table[256];
} wave[PXT_NO_MODELS];

double fgetv(FILE *fp) // Load a numeric value from text file; one per line.
{
  char Buf[4096], *p = Buf;
  Buf[4095] = '\0';
  if (!std::fgets(Buf, sizeof(Buf) - 1, fp))
    return 0.0;
  // Ignore empty lines. If the line was empty, try next line.
  if (!Buf[0] || Buf[0] == '\r' || Buf[0] == '\n')
    return fgetv(fp);
  while (*p && *p++ != ':')
  {
  }                         // Skip until a colon character.
  return std::strtod(p, 0); // Parse the value and return it.
}

bool stPXSound::load(const std::string &fname)
{
  FILE *fp;

  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    staterr("pxt->load: file '%s' not found.", fname.c_str());
    return false;
  }

  auto f  = [=]() { return (int32_t)fgetv(fp); };
  auto fu = [=]() { return (uint32_t)fgetv(fp); };

  for (auto &c : channels)
  {
    c = {
        f() != 0,
        fu(),                                       // enabled, length
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // carrier wave
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // frequency wave
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // amplitude wave
        {f(), {{f(), f()}, {f(), f()}, {f(), f()}}}, // envelope
        nullptr
    };
  }
  fclose(fp);
  return true;
}

void stPXSound::freeBuf()
{
  uint32_t i;

  // free up the buffers
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].buffer)
    {
      free(this->channels[i].buffer);
      this->channels[i].buffer = nullptr;
    }
  }

  if (this->final_buffer)
  {
    free(this->final_buffer);
    this->final_buffer = nullptr;
  }
}

int32_t stPXSound::allocBuf()
{
  uint32_t topbufsize = 64;
  uint32_t i;

  freeBuf();

  // allocate buffers for each enabled channel
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      this->channels[i].buffer = (signed char *)malloc(this->channels[i].nsamples);
      if (!this->channels[i].buffer)
      {
        staterr("AllocBuffers (pxt): out of memory (1)!");
        return -1;
      }

      if (this->channels[i].nsamples > topbufsize)
        topbufsize = this->channels[i].nsamples;
    }
  }

  // allocate the final buffer
  this->final_buffer = (signed char *)malloc(topbufsize);
  if (!this->final_buffer)
  {
    staterr("AllocBuffers (pxt): out of memory (2)!");
    return -1;
  }

  this->final_size = topbufsize;

  return topbufsize;
}

bool stPXSound::render()
{
  uint32_t i, s;
  int16_t mixed_sample;
  int16_t *middle_buffer;
  int32_t bufsize;

  bufsize = this->allocBuf();
  if (bufsize == -1)
    return 1; // error

  // --------------------------------
  //  render all the channels
  // --------------------------------
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      this->channels[i].synth();
    }
  }

  // ----------------------------------------------
  //  mix the channels [generate final_buffer]
  // ----------------------------------------------
  // lprintf("final_size = %d final_buffer = %08x\n", snd->final_size, snd->final_buffer);

  middle_buffer = (int16_t *)malloc(this->final_size * 2);

  memset(middle_buffer, 0, this->final_size * 2);

  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      for (s = 0; s < this->channels[i].nsamples; s++)
      {
        middle_buffer[s] += this->channels[i].buffer[s];
      }
    }
  }

  for (s = 0; s < this->final_size; s++)
  {
    mixed_sample = middle_buffer[s];

    if (mixed_sample > 127)
      mixed_sample = 127;
    else if (mixed_sample < -127)
      mixed_sample = -127;

    this->final_buffer[s] = (char)mixed_sample;
  }

  free(middle_buffer);
  return 0;
}

int32_t stPXEnvelope::evaluate(int32_t i) const // Linearly interpolate between the key points:
{
  int32_t prevval = initial, prevtime = 0;
  int32_t nextval = 0, nexttime = 256;
  for (int32_t j = 2; j >= 0; --j)
    if (i < p[j].time)
    {
      nexttime = p[j].time;
      nextval  = p[j].val;
    }
  for (int32_t j = 0; j <= 2; ++j)
    if (i >= p[j].time)
    {
      prevtime = p[j].time;
      prevval  = p[j].val;
    }
  if (nexttime <= prevtime)
    return prevval;
  return (i - prevtime) * (nextval - prevval) / (nexttime - prevtime) + prevval;
}

void stPXChannel::synth()
{
  if (!enabled)
    return;

  auto &c = carrier, &f = frequency, &a = amplitude;
  double mainpos = c.offset, maindelta = 256 * c.pitch / nsamples;
  for (size_t i = 0; i < nsamples; ++i)
  {
    auto s = [=](double p = 1) { return 256 * p * i / nsamples; };
    // Take sample from each of the three signal generators:
    int freqval = f.wave[0xFF & int(f.offset + s(f.pitch))] * f.level;
    int ampval  = a.wave[0xFF & int(a.offset + s(a.pitch))] * a.level;
    int mainval = c.wave[0xFF & int(mainpos)] * c.level;
    // Apply amplitude & envelope to the main signal level:
    buffer[i] = mainval * (ampval + 4096) / 4096 * envelope.evaluate(s()) / 4096;
    // Apply frequency modulation to maindelta:
    mainpos += maindelta * (1 + (freqval / (freqval < 0 ? 8192. : 2048.)));
  }
}

Pixtone::Pixtone() {}
Pixtone::~Pixtone() {}

Pixtone *Pixtone::getInstance()
{
  return Singleton<Pixtone>::get();
}

std::function<void(int chan)> sfxCallback;

void mySfxCallback(int chan)
{
  sfxCallback(chan);
}

bool Pixtone::init()
{
  if (_inited)
  {
    staterr("pxt_init: pxt module already initialized");
    return false;
  }
  else
    _inited = true;

  memset(_sound_fx, 0, sizeof(_sound_fx));
  for (uint16_t i = 0; i < 256; i++)
    _sound_fx[i].channel = -1;

  for (uint32_t seed = 0, i = 0; i < 256; ++i)
  {
    seed                       = (seed * 214013) + 2531011;          // Linear congruential generator
    wave[MOD_SINE].table[i]    = 0x40 * std::sin(i * 3.1416 / 0x80); // Sine
    wave[MOD_TRI].table[i]     = ((0x40 + i) & 0x80) ? 0x80 - i : i; // Triangle
    wave[MOD_SAWUP].table[i]   = -0x40 + i / 2;                      // Sawtooth up
    wave[MOD_SAWDOWN].table[i] = 0x40 - i / 2;                       // Sawtooth down
    wave[MOD_SQUARE].table[i]  = 0x40 - (i & 0x80);                  // Square
    wave[MOD_NOISE].table[i]   = (signed char)(seed >> 16) / 2;      // Pseudorandom
  }

  char fname[80];
  uint32_t slot;

  stat("Loading Sound FX...");

  std::string path = ResourceManager::getInstance()->getPathForDir("pxt/");
  for (slot = 1; slot <= NUM_SOUNDS; slot++)
  {
    sprintf(fname, "%sfx%02x.pxt", path.c_str(), slot);
    stPXSound snd;

    if (!snd.load(fname))
      continue;
    snd.render();

    // upscale the sound to 16-bit for SDL_mixer then throw away the now unnecessary 8-bit data
    _prepareToPlay(&snd, slot);
    snd.freeBuf();
  }

  sfxCallback = std::bind(&Pixtone::pxtSoundDone, this, std::placeholders::_1);
  Mix_ChannelFinished(mySfxCallback);

  return true;
}

void Pixtone::shutdown()
{
  for (uint32_t i = 0; i <= NUM_SOUNDS; i++)
  {
    if (_sound_fx[i].chunk)
    {
      SDL_free(_sound_fx[i].chunk->abuf);
      Mix_FreeChunk(_sound_fx[i].chunk);
      _sound_fx[i].chunk = nullptr;
    }
    if (_sound_fx[i].resampled)
    {
      SDL_free(_sound_fx[i].resampled->abuf);
      Mix_FreeChunk(_sound_fx[i].resampled);
      _sound_fx[i].resampled = nullptr;
    }
  }
}

int Pixtone::play(int32_t chan, int32_t slot, int32_t loop)
{
  if (_sound_fx[slot].chunk)
  {
    chan                    = Mix_PlayChannel(chan, _sound_fx[slot].chunk, loop);
    _sound_fx[slot].channel = chan;
    _slots[chan]            = slot;

    if (chan < 0)
    {
      staterr("pxt_Play: SSPlayChunk returned error");
    }
    return chan;
  }
  else
  {
    staterr("pxt_Play: sound slot 0x%02x not rendered", slot);
    return -1;
  }
}

int Pixtone::playResampled(int32_t chan, int32_t slot, int32_t loop, uint32_t percent)
{
  if (_sound_fx[slot].chunk)
  {
    uint32_t resampled_rate = SAMPLE_RATE * (percent / 100);

    if (resampled_rate != _sound_fx[slot].resampled_rate)
    {
      SDL_AudioCVT cvt;

      if (SDL_BuildAudioCVT(&cvt, AUDIO_S16, 2, SAMPLE_RATE, AUDIO_S16, 2, resampled_rate) == -1)
      {
        staterr("SDL_BuildAudioCVT: %s\n", SDL_GetError());
      }
      cvt.len = _sound_fx[slot].chunk->alen;
      cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
      memcpy(cvt.buf, _sound_fx[slot].chunk->abuf, _sound_fx[slot].chunk->alen);

      if (SDL_ConvertAudio(&cvt) == -1)
      {
        printf("SDL_ConvertAudio: %s\n", SDL_GetError());
      }

      if (_sound_fx[slot].resampled != NULL)
      {
        SDL_free(_sound_fx[slot].resampled);
        _sound_fx[slot].resampled = NULL;
      }
      _sound_fx[slot].resampled = Mix_QuickLoad_RAW((Uint8 *)cvt.buf, cvt.len_cvt);
      _sound_fx[slot].resampled_rate = resampled_rate;
    }

    chan                    = Mix_PlayChannel(chan, _sound_fx[slot].resampled, loop);
    _sound_fx[slot].channel = chan;
    _slots[chan]            = slot;

    if (chan < 0)
    {
      staterr("pxt_Play: SSPlayChunk returned error");
    }
    return chan;
  }
  else
  {
    staterr("pxt_Play: sound slot 0x%02x not rendered", slot);
    return -1;
  }
}

void Pixtone::stop(int32_t slot)
{
  if (_sound_fx[slot].channel != -1)
  {
    Mix_HaltChannel(_sound_fx[slot].channel);
    if (_sound_fx[slot].channel != -1)
    {
      _slots[_sound_fx[slot].channel] = -1;
    }
  }
}

void Pixtone::pxtSoundDone(int channel)
{
  if (_slots[channel] != -1)
  {
    _sound_fx[_slots[channel]].channel = -1;
  }
}

void Pixtone::_prepareToPlay(stPXSound *snd, int32_t slot)
{
  // convert the buffer from 8-bit mono signed to 16-bit stereo signed
  SDL_AudioCVT cvt;

  if (SDL_BuildAudioCVT(&cvt, AUDIO_S8, 1, 22050, AUDIO_S16, 2, SAMPLE_RATE) == -1)
  {
    staterr("SDL_BuildAudioCVT: %s\n", SDL_GetError());
  }

  cvt.len = snd->final_size;
  cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
  memcpy(cvt.buf, snd->final_buffer, snd->final_size);

  if (SDL_ConvertAudio(&cvt) == -1)
  {
    printf("SDL_ConvertAudio: %s\n", SDL_GetError());
  }

  _sound_fx[slot].chunk = Mix_QuickLoad_RAW((Uint8 *)cvt.buf, cvt.len_cvt);
}

} // namespace Sound
} // namespace NXE