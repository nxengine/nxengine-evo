/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#ifndef _PIXTONE_H
#define _PIXTONE_H

#include "../Singleton.h"
#include "../common/basics.h"
#include "SoundManager.h"

#include <SDL_mixer.h>
#include <string>

#define PXT_NO_CHANNELS 4
#define PXENV_NUM_VERTICES 3

#define NUM_RESAMPLED_BUFFERS 16

namespace NXE
{
namespace Sound
{

enum
{
  MOD_SINE,
  MOD_TRI,
  MOD_SAWUP,
  MOD_SAWDOWN,
  MOD_SQUARE,
  MOD_NOISE,

  PXT_NO_MODELS
};

typedef struct
{
  int32_t initial;
  struct
  {
    int32_t time, val;
  } p[PXENV_NUM_VERTICES];
  int32_t evaluate(int32_t i) const;
} stPXEnvelope;

typedef struct
{
  bool enabled;
  uint32_t nsamples;
  struct stPXWave
  {
    const int8_t *wave;
    double pitch;
    int32_t level, offset;
  };

  stPXWave carrier;
  stPXWave frequency;
  stPXWave amplitude;
  stPXEnvelope envelope;
  int8_t *buffer;
  void synth();
} stPXChannel;

typedef struct
{
  stPXChannel channels[PXT_NO_CHANNELS];

  int8_t *final_buffer = nullptr;
  uint32_t final_size;
  bool load(const std::string &fname);
  bool render();
  int32_t allocBuf();
  void freeBuf();
} stPXSound;

class Pixtone
{
public:
  static Pixtone *getInstance();
  bool init();
  void shutdown();

  int play(int32_t chan, int32_t slot, int32_t loop);
  int playResampled(int32_t chan, int32_t slot, int32_t loop, uint32_t percent);
  int prepareResampled(int32_t slot, uint32_t percent);
  void stop(int32_t slot);

  void pxtSoundDone(int channel);

protected:
  friend class Singleton<Pixtone>;

  Pixtone();
  ~Pixtone();
  Pixtone(const Pixtone &) = delete;
  Pixtone &operator=(const Pixtone &) = delete;

private:
  void _prepareToPlay(stPXSound *snd, int32_t slot);
  bool _inited = false;
  struct
  {
    Mix_Chunk *chunk         = NULL;
    Mix_Chunk *resampled[NUM_RESAMPLED_BUFFERS] = {NULL};
    uint32_t resampled_rate[NUM_RESAMPLED_BUFFERS]  = {SAMPLE_RATE};
    int32_t channel          = -1;
  } _sound_fx[256];
  int32_t _slots[64];
  const uint32_t NUM_SOUNDS = 0x75;
};

} // namespace Sound
} // namespace NXE
#endif
