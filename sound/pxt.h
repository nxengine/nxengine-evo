
#ifndef _PXT_H
#define _PXT_H

#include "../common/basics.h"

#define PXT_NO_CHANNELS			4

enum
{
	MOD_SINE,
	MOD_TRI,
	MOD_SAWUP,
	MOD_SAWDOWN,
	MOD_SQUARE,
	MOD_NOISE,
	MOD_WHITE,
	MOD_PULSE,
	
	PXT_NO_MODELS
};

typedef struct
{
	signed char *model;		// ptr to model data
	uint8_t model_no;			// index of model data (0-5, which wave[] model points to)
	
	double phaseacc;
	double phaseinc;
	double repeat;			// pixtone calls it "freq"
	uint8_t volume;			// pixtone calls it "top"
	uint8_t offset;
	
	int white_ptr;			// like "phaseacc" but for MOD_WHITE (the odd one out)
} stPXWave;

#define PXENV_NUM_VERTICES		3
typedef struct
{
	int initial;
	
	int time[PXENV_NUM_VERTICES];
	int val[PXENV_NUM_VERTICES];
} stPXEnvelope;

typedef struct
{
	int size_blocks;
	char enabled;
	
	stPXWave main;
	stPXWave pitch;
	stPXWave pitch2;
	stPXWave volume;
	stPXEnvelope envelope;
	
	unsigned char envbuffer[256];
	signed char *buffer;
} stPXChannel;

typedef struct
{
	stPXChannel chan[PXT_NO_CHANNELS];
	
	signed char *final_buffer;
	int final_size;
} stPXSound;

static unsigned short rand_next(void);
static void GenerateSineModel(unsigned char *table);
static void GenerateTriangleModel(unsigned char *table);
static void GenerateSawUpModel(unsigned char *table);
static void GenerateSawDownModel(unsigned char *table);
static void GenerateSquareModel(unsigned char *table);
static void GenerateRandModel(unsigned char *table);
void GenerateWhiteModel(void);
static void GeneratePulseModel(unsigned char *table);
char pxt_init(void);
char pxt_initsynth(void);
char pxt_SetModel(stPXWave *pxwave, int m);
void pxt_SetDefaultEnvelope(stPXEnvelope *env);
void GenerateEnvelope(stPXEnvelope *env, char *buffer);
void pxt_RenderPXWave(stPXWave *pxwave, signed char *buffer, int size_blocks);
static void CreateAudio(stPXChannel *chan);
static char AllocBuffers(stPXSound *snd);
char pxt_Render(stPXSound *snd);
void pxt_PrepareToPlay(stPXSound *snd, int slot);
void pxt_ChangePitch(stPXSound *snd, double factor);
int pxt_Play(int chan, int slot, char loop);
int pxt_PlayWithCallback(int chan, int slot, char loop, void (*FinishedCB)(int, int));
static void pxtSoundDone(int chan, int slot);
static void pxtLooper(int chan, int slot);
void pxt_Stop(int slot);
char pxt_IsPlaying(int slot);
char pxt_LoadSoundFX(const char *path, const char *cache_name, int top);
static char LoadFXCache(const char *fname, int top);
void pxt_freeSoundFX(void);
void pxt_FreeSound(int slot);
void FreePXTBuf(stPXSound *snd);
char pxt_load(const char *fname, stPXSound *snd);
static char LoadComponent(FILE *fp, stPXWave *pxw);
static char ReadToBracket(FILE *fp);
char pxt_save(const char *fname, stPXSound *snd);
static void SaveComponent(FILE *fp, const char *name, stPXWave *pxw);
static void SaveComponentMachine(FILE *fp, stPXWave *pxw, char trailcomma);
static void SaveEnvVertice(FILE *fp, stPXEnvelope *env, int v);



#endif

