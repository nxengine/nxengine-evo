
// PXT sound file player
// see bottom of file for info on how to use this module
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <cmath>			// for sin()
#include <cstdlib>
#include <cstring>

#include "../config.h"
#include "pxt.h"
#include "sound.h"
//#include "sslib.h"
#include "../common/stat.h"
#include "../common/misc.h"


#define MODEL_SIZE			256
//#define PXCACHE_MAGICK		'PXC1'
#define PXCACHE_MAGICK           ( ( '1' << 24 ) + ( 'C' << 16 ) + ( 'X' << 8 ) + 'P' )




#define WHITE_LEN		SAMPLE_RATE
int8_t white[WHITE_LEN];

// the final sounds ready to play (after pxt_PrepareToPlay)
static struct
{
	Mix_Chunk* chunk = NULL;
	Mix_Chunk* resampled = NULL;
	unsigned int resampled_rate = SAMPLE_RATE;
	int channel = -1;
} sound_fx[256];

static int slots[64];

static int load_top;

static struct
{
	uint8_t table[256];
} wave[PXT_NO_MODELS];


// gets the next byte from wave "wave", scales it by the waves volume, and places result in "out".
// x * (y / z) = (x * y) / z
inline void GETWAVEBYTE(stPXWave* wave, int& out)
{	
	if (wave->model_no != MOD_WHITE)	
	{		
	    unsigned char index = static_cast<unsigned char>(static_cast<unsigned int>(wave->phaseacc) % 256);
		out = wave->model[index];	
	}		
	else	
	{	
		out = white[wave->white_ptr];		
		if (++wave->white_ptr >= WHITE_LEN) wave->white_ptr = 0;	
	}	
	out *= wave->volume;			
	out /= 64;	
}


static unsigned int rng_seed = 0;

static unsigned short rand_next(void)
{
	rng_seed *= 0x343fd;
	rng_seed += 0x269ec3;
	
	return (rng_seed >> 16) & 0x7fff;
}

static void GenerateSineModel(unsigned char *table)
{
	double twopi = 6.283184000f;
	double ratio = 256.00f;
	double rat64 = 64.00f;
	double reg;
	int i;

	for(i=0;i<256;i++)
	{
		reg = (double)i;
		reg *= twopi;
		reg /= ratio;
		reg = sin(reg);
		reg *= rat64;
		
		table[i] = (unsigned char)reg;
	}
}


static void GenerateTriangleModel(unsigned char *table)
{
	int i, f;

	for(i=0;i<64;i++) table[i] = i;
	
	f = 0;
	for(;i<192;i++)
	{
		table[i] = 0x40 - f;
		f++;
	}
	
	f = 0;
	for(;i<256;i++)
	{
		table[i] = f - 0x40;
		f++;
	}
}


static void GenerateSawUpModel(unsigned char *table)
{
	int i;

	for(i=0;i<256;i++)
		table[i] = (i >> 1) - 0x40;
}


static void GenerateSawDownModel(unsigned char *table)
{
	int i;

	for(i=0;i<256;i++)
		table[i] = 0x40 - (i >> 1);
}


static void GenerateSquareModel(unsigned char *table)
{
	int i;

	for(i=0;i<128;i++) table[i] = 0x40;
	for(;i<256;i++) table[i] = (uint8_t)-0x40;
}


static void GenerateRandModel(unsigned char *table)
{
	int i;
	signed char k;

	rng_seed = 0;
	
	for(i=0;i<256;i++)
	{
		k = (signed char)rand_next();
		
		if (k < 0) k++;
		table[i] = k >> 1;
	}
}

void GenerateWhiteModel(void)
{
	int i;

	seedrand(0xa42c1911);
	
	for(i=0;i<WHITE_LEN;i++)
		white[i] = random(-63, 63);
}

static void GeneratePulseModel(unsigned char *table)
{
	int i;

	for(i=0;i<192;i++) table[i] = 0x40;
	for(;i<256;i++) table[i] = (uint8_t)-0x40;
}


// generate the models so we can do synth
// must call this before doing any rendering
char pxt_init(void)
{
	static int inited = 0;
	int i;

	if (inited)
	{
		staterr("pxt_init: pxt module already initialized");
		return 0;
	}
	else inited = 1;
	
	memset(sound_fx, 0, sizeof(sound_fx));
	for(i=0;i<256;i++) sound_fx[i].channel = -1;
	
	return 0;
}

char pxt_initsynth(void)
{
	static int synth_inited = 0;
	if (synth_inited) return 0; else synth_inited = 1;
	
	GenerateSineModel(wave[MOD_SINE].table);
	GenerateTriangleModel(wave[MOD_TRI].table);
	GenerateSawUpModel(wave[MOD_SAWUP].table);
	GenerateSawDownModel(wave[MOD_SAWDOWN].table);
	GenerateSquareModel(wave[MOD_SQUARE].table);
	GenerateRandModel(wave[MOD_NOISE].table);
	GeneratePulseModel(wave[MOD_PULSE].table);
	GenerateWhiteModel();
	return 0;
}

char pxt_SetModel(stPXWave *pxwave, int m)
{
	if (m >= 0 && m < PXT_NO_MODELS)
	{
		pxwave->model = (signed char *)wave[m].table;
		pxwave->model_no = m;
		return 0;
	}
	else
	{
		staterr("pxt_SetModel: invalid sound model '%d'", m);
		return 1;
	}
}


// sets the given envelope to default values
void pxt_SetDefaultEnvelope(stPXEnvelope *env)
{
	env->initial = 63;
	env->time[0] = 64;
	env->time[1] = 128;
	env->time[2] = 255;
	env->val[0] = 63;
	env->val[1] = 63;
	env->val[2] = 63;
}


// generate a 256-byte envelope "waveform" containing volume adjustment values from 00-3f
// in short it renders the envelope for a sound.
// the envelope must be ready before CreateAudio can be used.
void GenerateEnvelope(stPXEnvelope *env, char *buffer)
{
	double curenv, envinc;
	int i;

	curenv = env->initial;
	envinc = (double)(env->val[0] - env->initial) / env->time[0];
	for(i=0;i<env->time[0];i++)
	{
		buffer[i] = (int)curenv;
		curenv += envinc;
	}
	
	curenv = env->val[0];
	envinc = (double)(env->val[1] - env->val[0]) / (env->time[1] - env->time[0]);
	for(;i<env->time[1];i++)
	{
		buffer[i] = (int)curenv;
		curenv += envinc;
	}
	
	curenv = env->val[1];
	envinc = (double)(env->val[2] - env->val[1]) / (env->time[2] - env->time[1]);
	for(;i<env->time[2];i++)
	{
		buffer[i] = (int)curenv;
		curenv += envinc;
	}
	
	// fade to 0 volume if time_c is < end of sound, just like the pretty drawing in PixTone.
	envinc = (double)(-1 - env->val[2]) / (256 - env->time[2]);
	curenv = env->val[2];
	for(;i<256;i++)
	{
		buffer[i] = (int)curenv;
		curenv += envinc;
	}
}

// renders a sound channel.
// call GenerateEnvelope first.
static void CreateAudio(stPXChannel *chan)
{
	// store all the commonly-used pointers
	stPXWave *main = &chan->main;
	stPXWave *pitch = &chan->pitch;
	stPXWave *pitch2 = &chan->pitch2;
	stPXWave *volume = &chan->volume;
	unsigned char *envbuffer = chan->envbuffer;
	int size_blocks = chan->size_blocks;
	// var defs
	int i, j;
	int output, bm, bm2, volmod;
	double phaseval;
	int e;
	double env_acc, env_inc;

	// we generate twice the buf size and average it down afterwards for increased precision.
	// this is what pixtone does, and although I'm not sure if that's why; it really does
	// seem to make a slight quality increase
	size_blocks *= 2;
	signed char *buffer = (signed char *)malloc(size_blocks);
	
	//lprintf("CreateAudio: buffer len %d, repeat = %.2f / %.2f\n", size_blocks, main->repeat, pitch->repeat);
	
	// calculate all the phaseinc's
	main->phaseinc = ((MODEL_SIZE * main->repeat) / (double)size_blocks);
	pitch->phaseinc = ((MODEL_SIZE * pitch->repeat) / (double)size_blocks);
	pitch2->phaseinc = ((MODEL_SIZE * pitch2->repeat) / (double)size_blocks);
	volume->phaseinc = ((MODEL_SIZE * volume->repeat) / (double)size_blocks);
	env_inc = (MODEL_SIZE / (double)size_blocks);
	
	//lprintf("main phaseinc = %.6f\n", main->phaseinc);
	//lprintf("pitch phaseinc = %.6f\n", pitch->phaseinc);
	//lprintf("volume phaseinc = %.6f\n", volume->phaseinc);
	
	// set the starting positions
	main->phaseacc = (double)main->offset;
	pitch->phaseacc = (double)pitch->offset;
	pitch2->phaseacc = (double)pitch2->offset;
	volume->phaseacc = (double)volume->offset;
	
	main->white_ptr = main->offset;
	pitch->white_ptr = pitch->offset;
	pitch2->white_ptr = pitch2->offset;
	volume->white_ptr = volume->offset;
	
	env_acc = 0;
	
	for(i=0;i<size_blocks;i++)
	{
		// get next sample from the main waveform
		GETWAVEBYTE(main, output);
		GETWAVEBYTE(volume, volmod);
		
		// hack to allow inverting sign of MOD_PULSE-based volume modulators
		// by just raising the top over 128
		if (volume->model_no==MOD_PULSE)
		{
			if (volmod > 127 || volmod < -127)
			{
				if (volmod > 127)
				{
					volmod = 256 - volmod;
				}
				else
				{
					volmod = -(256 - -volmod);
				}
				volmod = -volmod;
			}
		}
		
		// offset volume modulator such that it completely silences the sound at <= -0x40
		volmod += 64;
		if (volmod < 0) volmod = 0;
		
		// apply volume modulator to main
		output = (output * volmod) / 64;
		
		// apply the envelope
		e = envbuffer[(unsigned char)env_acc];
		output = (output * e) / 64;
		
		// save sample to output buffer
		buffer[i] = output;
		
		
		// get next byte from the frequency modulator waveform
		GETWAVEBYTE(pitch, bm);
		GETWAVEBYTE(pitch2, bm2);
		
		// clip the values to a signed char if it's a pulse waveform...allows
		// switch it's signed from up/up/down to down/down/up just by taking
		// the top over 128. but for other waveforms we actually want the clipping
		// to not happen right in this case because A. it allows for making larger
		// range frequency sweeps than otherwise possible and B. pixtone has this
		// same "glitch" so it's a compatibility thing.
		if (pitch->model_no==MOD_PULSE) bm = (signed char)bm;
		if (pitch2->model_no==MOD_PULSE) bm2 = (signed char)bm2;
		
		bm += bm2;
		
		
		if (bm >= 0)
		{	// when positive, every 32 clicks doubles the phaseinc.
			// this is actually "phaseval = (mod / 32) * main->phaseinc;" however
			// we lose precision by doing the division first, so this is equivalent:
			phaseval = ((double)bm * main->phaseinc) / 32;
			
			// phaseinc is sped up by phaseval
			main->phaseacc += (main->phaseinc + phaseval);
		}
		else
		{	// when negative, every 64 clicks half's the phaseinc.
			// this can be thought of as "phaseval = (mod / 64) * (main->phaseinc * 0.5f);" however
			// that doesn't actually work because of precision loss, so this instead:
			phaseval = ((double)(-bm) * main->phaseinc) / 128;
			
			// phaseinc is slowed down by phaseval
			main->phaseacc += (main->phaseinc - phaseval);
		}
		
		
		pitch->phaseacc += pitch->phaseinc;
		pitch2->phaseacc += pitch2->phaseinc;
		volume->phaseacc += volume->phaseinc;
		
		env_acc += env_inc;
		// just to make certain the envelope never starts over from the beginning.
		// although it shouldn't; the precision on env_inc seems to be pretty good.
		if (env_acc > 255) env_acc = 255;
		
		// normally we would have to do this, but we don't as long as model_size is 256
		// and we cast phaseacc to an unsigned char when we do the lookup; it'll wrap by itself.
		// neat huh.
		//while(main->phaseacc >= MODEL_SIZE) main->phaseacc -= MODEL_SIZE;
		//while(pitch->phaseacc >= MODEL_SIZE) pitch->phaseacc -= MODEL_SIZE;
	}
	
	// average our doublesampled audio down into the final buffer
	signed char *outbuffer = chan->buffer;
	for(i=j=0;(i+1)<size_blocks;i+=2)
	{
		e = buffer[i] + buffer[i+1];
		e >>= 1;
		outbuffer[j++] = e;
	}
	free(buffer);
}


// allocate all the buffers needed for the given sound
static char AllocBuffers(stPXSound *snd)
{
	int topbufsize = 64;
	int i;

	FreePXTBuf(snd);
	
	// allocate buffers for each enabled channel
	for(i=0;i<PXT_NO_CHANNELS;i++)
	{
		if (snd->chan[i].enabled)
		{
			snd->chan[i].buffer = (signed char *)malloc(snd->chan[i].size_blocks);
			if (!snd->chan[i].buffer)
			{
				staterr("AllocBuffers (pxt): out of memory (1)!");
				return -1;
			}
			
			if (snd->chan[i].size_blocks > topbufsize)
				topbufsize = snd->chan[i].size_blocks;
		}
	}
	
	// allocate the final buffer
	snd->final_buffer = (signed char *)malloc(topbufsize);
	if (!snd->final_buffer)
	{
		staterr("AllocBuffers (pxt): out of memory (2)!");
		return -1;
	}
	
	snd->final_size = topbufsize;
	
	return topbufsize;
}


// generate 8-bit signed PCM audio from a PXT sound, put it in it's final_buffer.
char pxt_Render(stPXSound *snd)
{
	int i, s;
	signed short mixed_sample;
	signed short *middle_buffer;
	int bufsize;

	bufsize = AllocBuffers(snd);
	if (bufsize==-1) return 1;			// error
	
	// --------------------------------
	//  render all the channels
	// --------------------------------
	for(i=0;i<PXT_NO_CHANNELS;i++)
	{
		if (snd->chan[i].enabled)
		{
//			memset(snd->chan[i].buffer, 0, sizeof(snd->chan[i].buffer));
			GenerateEnvelope(&snd->chan[i].envelope, (char *)snd->chan[i].envbuffer);
			CreateAudio(&snd->chan[i]);
		}
	}
	
	// ----------------------------------------------
	//  mix the channels [generate final_buffer]
	// ----------------------------------------------
	//lprintf("final_size = %d final_buffer = %08x\n", snd->final_size, snd->final_buffer);
	
	middle_buffer = (signed short *)malloc(snd->final_size * 2);
	memset(middle_buffer, 0, snd->final_size * 2);
	
	for(i=0;i<PXT_NO_CHANNELS;i++)
	{
		if (snd->chan[i].enabled)
		{
			for(s=0;s<snd->chan[i].size_blocks;s++)
			{
				middle_buffer[s] += snd->chan[i].buffer[s];
			}
		}
	}
	
	for(s=0;s<snd->final_size;s++)
	{
		mixed_sample = middle_buffer[s];
		
		if (mixed_sample > 127) mixed_sample = 127;
		else if (mixed_sample < -127) mixed_sample = -127;
		
		snd->final_buffer[s] = (char)mixed_sample;
	}
	
	free(middle_buffer);
	return 0;
}


// get an already-rendered pxt 'snd' ready for sending to SDL_mixer.
// converts the 8-bit signed audio to SDL_mixer's 16-bit stereo format and
// sets up the Mix_Chunk.
void pxt_PrepareToPlay(stPXSound *snd, int slot)
{
	int value;
	int ap;
	int i;
	signed char *buffer = snd->final_buffer;
	signed short *outbuffer;
	int malc_size;

	// convert the buffer from 8-bit mono signed to 16-bit stereo signed
	malc_size = (snd->final_size * 2 * 2);
	outbuffer = (signed short *)malloc(malc_size);
	
	for(i=ap=0;i<snd->final_size;i++)
	{
		value = buffer[i];
		value *= 200;
		value = SDL_SwapLE16(value);
		
		outbuffer[ap++] = value;		// left ch
		outbuffer[ap++] = value;		// right ch
	}
	
	
//	sound_fx[slot].buffer = outbuffer;
//	sound_fx[slot].len = snd->final_size;
	sound_fx[slot].chunk = Mix_QuickLoad_RAW((Uint8*)outbuffer, malc_size);
	
	//lprintf("pxt ready to play in slot %d\n", slot);
}

void pxtSoundDone(int channel)
{
    if (slots[channel] != -1)
    {
      sound_fx[slots[channel]].channel=-1;
    }
}

int pxt_Play(int chan, int slot, char loop)
{
	if (sound_fx[slot].chunk)
	{
		chan = Mix_PlayChannel(chan, sound_fx[slot].chunk, loop);
		sound_fx[slot].channel = chan;
		slots[chan]=slot;

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

int pxt_PlayResampled(int chan, int slot, char loop, int percent)
{
	if (sound_fx[slot].chunk)
	{
		unsigned int resampled_rate = SAMPLE_RATE * (percent / 100);

		if (resampled_rate != sound_fx[slot].resampled_rate)
		{
			SDL_AudioCVT cvt;
			
			if (SDL_BuildAudioCVT(&cvt, AUDIO_S16, 2, SAMPLE_RATE, AUDIO_S16, 2, resampled_rate) == -1)
			{
				staterr("SDL_BuildAudioCVT: %s\n",SDL_GetError());
			}
			cvt.len = sound_fx[slot].chunk->alen;
			cvt.buf = (Uint8 *) SDL_malloc(cvt.len * cvt.len_mult);
			memcpy(cvt.buf, sound_fx[slot].chunk->abuf, sound_fx[slot].chunk->alen);
			
			if (SDL_ConvertAudio(&cvt) == -1)
			{
				printf("SDL_ConvertAudio: %s\n",Mix_GetError());
			}
			if (sound_fx[slot].resampled != NULL)
			{
				delete sound_fx[slot].resampled;
			}
			sound_fx[slot].resampled = new Mix_Chunk;
			sound_fx[slot].resampled->allocated=sound_fx[slot].chunk->allocated;
			sound_fx[slot].resampled->abuf = (Uint8 *) SDL_malloc(cvt.len_cvt);
			sound_fx[slot].resampled->alen = cvt.len_cvt;
			sound_fx[slot].resampled->volume = 128;
			memcpy(sound_fx[slot].resampled->abuf, cvt.buf, cvt.len_cvt);
			SDL_free(cvt.buf);
		}
		chan = Mix_PlayChannel(chan, sound_fx[slot].resampled, loop);
		sound_fx[slot].channel = chan;
		slots[chan]=slot;

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


void pxt_Stop(int slot)
{
	if (sound_fx[slot].channel != -1)
	{
		Mix_HaltChannel(sound_fx[slot].channel);
		if (sound_fx[slot].channel != -1)
		{
			slots[sound_fx[slot].channel]=-1;
		}

	}
}

char pxt_IsPlaying(int slot)
{
	return (sound_fx[slot].channel != -1);
}




// render all pxt files under "path" up to slot "top".
// get them all ready to play in their sound slots.

char pxt_LoadSoundFX(const std::string& path, int top)
{
	char fname[80];
	int slot;
	stPXSound snd;

	stat("Loading Sound FX...");
	load_top = top;
	
	
	// get ready to do synthesis
	pxt_initsynth();
	
	for(slot=1;slot<=top;slot++)
	{
		sprintf(fname, "%sfx%02x.pxt", path.c_str(), slot);
		
		if (pxt_load(fname, &snd)) continue;
		pxt_Render(&snd);
		
		// upscale the sound to 16-bit for SDL_mixer then throw away the now unnecessary 8-bit data
		pxt_PrepareToPlay(&snd, slot);
		FreePXTBuf(&snd);
	}
	
	return 0;
}



void pxt_freeSoundFX(void)
{
	int i;
	for(i=0;i<=load_top;i++)
	{
		if (sound_fx[i].chunk)
		{
			Mix_FreeChunk(sound_fx[i].chunk);
			sound_fx[i].chunk = NULL;
//			free(sound_fx[i].buffer);
//			sound_fx[i].buffer = NULL;
		}
	}
}

// free all the internal buffers of a PXSound
void FreePXTBuf(stPXSound *snd)
{
	if (snd)
	{
		int i;
		
		// free up the buffers
		for(i=0;i<PXT_NO_CHANNELS;i++)
		{
			if (snd->chan[i].buffer)
			{
				free(snd->chan[i].buffer);
				snd->chan[i].buffer = NULL;
			}
		}
		
		if (snd->final_buffer)
		{
			free(snd->final_buffer);
			snd->final_buffer = NULL;
		}
	}
}

static char LoadComponent(FILE *fp, stPXWave *pxw)
{
	if (pxt_SetModel(pxw, fgeticsv(fp))) return 1;
	
	pxw->repeat = fgetfcsv(fp);
	pxw->volume = fgeticsv(fp);
	pxw->offset = fgeticsv(fp);
	return 0;
}

static char ReadToBracket(FILE *fp)
{
uint8_t ch;

	for(;;)
	{
		ch = fgetc(fp);
		if (ch=='{') break;
		
		if (feof(fp))
		{
			staterr("pxt_load: file is in incorrect file format [failed to find '{']");
			fclose(fp);
			return 1;
		}
	}
	return 0;
}


// read a .pxt file into memory and return a stPXSound ready to be rendered.
char pxt_load(const std::string& fname, stPXSound *snd)
{
	FILE *fp;
	char load_extended_section = 0;
	char ch;
	int i, cc;
	
	
	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp) { staterr("pxt_load: file '%s' not found.", fname.c_str()); return 1; }
	
	//lprintf("pxt_load: reading %s...\n", fname);
	
	// set all buffers to non-existant and zero-length to start
	memset(snd, 0, sizeof(stPXSound));
	
	
	// skip ahead in the file till we find the machine readable data, denoted by '{'
	if (ReadToBracket(fp)) return 1;
	
	// load all channels we find
	cc = 0;
	ch = '{';
	while(!feof(fp))
	{
		if (ch=='>')		// extended section
		{
			load_extended_section = 1;
			break;
		}
		else if (ch=='{')
		{	// opening a new channel
			if (cc >= PXT_NO_CHANNELS)
			{
				staterr("pxt_load: sound '%s' contains too many channels!", fname.c_str());
				goto error;
			}
			
			snd->chan[cc].enabled = fgeticsv(fp);
			snd->chan[cc].size_blocks = fgeticsv(fp);
			
			if (LoadComponent(fp, &snd->chan[cc].main)) goto error;
			if (LoadComponent(fp, &snd->chan[cc].pitch)) goto error;
			if (LoadComponent(fp, &snd->chan[cc].volume)) goto error;
			
			snd->chan[cc].envelope.initial = fgeticsv(fp);
			for(i=0;i<PXENV_NUM_VERTICES;i++)
			{
				snd->chan[cc].envelope.time[i] = fgeticsv(fp);
				snd->chan[cc].envelope.val[i] = fgeticsv(fp);
			}
			
			cc++;
		}
		
		ch = fgetc(fp);
	}
	
	if (load_extended_section)
	{
		stat("pxt_load: extended section found, loading it");
		if (ReadToBracket(fp)) return 1;
		for(cc=0;cc<PXT_NO_CHANNELS;cc++)
		{
			if (LoadComponent(fp, &snd->chan[cc].pitch2)) goto error;
		}
	}
	else
	{
		//stat("No extended section found; setting compatibility values.");
		for(i=0;i<PXT_NO_CHANNELS;i++)
		{
			memset(&snd->chan[i].pitch2, 0, sizeof(stPXWave));
			pxt_SetModel(&snd->chan[i].pitch2, 0);
		}
	}
	
	//stat("pxt_load: '%s' parsed ok", fname.c_str());
	fclose(fp);
	return 0;
	
error: ;
	for(i=0;i<PXT_NO_CHANNELS;i++)
	{
		if (snd->chan[i].buffer)
		{
			free(snd->chan[i].buffer);
			snd->chan[i].buffer = NULL;
		}
	}
	if (fp) fclose(fp);
	
	return 1;
}




