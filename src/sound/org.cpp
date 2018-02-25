
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

#include "../common/basics.h"
#include "org.h"
#include "pxt.h"			// for loading drums
//#include "sslib.h"			// SAMPLE_RATE
#include "sound.h"			// SAMPLE_RATE
#include "../common/stat.h"
#include "../common/misc.h"


//#define QUIET
#define DRUM_PXT

#ifdef DRUM_PXT
	#define drumK		SAMPLE_RATE
#else
	#define drumK		30050
#endif

static bool org_inited = false;

static stNoteChannel note_channel[16];

static stSong song;

static int cache_ahead_time = 2000;		// approximate number of ms to cache ahead (is rounded to a # of beats)

static int buffer_beats;				// # of beats to cache ahead in each buffer
static int buffer_samples;				// how many samples are in each outbuffer
static int outbuffer_size_bytes;		// length of each outbuffer, and of final_buffer, in bytes

static struct
{
	signed short *samples;		// pointer to the raw PCM sound data
	int firstbeat;				// beat # of the first beat contained in this chunk
} final_buffer;

static int OrgVolume;

static struct
{
	signed short samples[256];
} wavetable[100];

#ifdef DRUM_PXT
	// sound effect numbers which correspond to the drums
	static const unsigned char drum_pxt[] =
	{
		0x96, 0, 0x97, 0, 0x9a, 0x98,
		0x99, 0, 0x9b, 0, 0, 0
	};
#else
	// names of the WAV files to load for each drum slot
	static const char *drum_names[] =
	{
		"Bass01", "Bass02", "Snare01", "Snare02", "Tom01", "HiClose",
		"HiOpen", "Crash", "Per01", "Per02", "Bass03", "Tom02"
	};
#endif

static struct
{
	signed short *samples;
	int nsamples;
} drumtable[NUM_DRUMS];

static int pitch[NUM_NOTES];


static void init_pitch(void)
{
	stat("Calculating pitch scale...");
	
	for(int i=0;i<NUM_NOTES;i++)
	{
		pitch[i] = (int)(441.0*(pow(2.0,((i-19.0)/12.0))));
	}
}


// given an instrument pitch and a note, returns the sampling rate that the
// wavetable sample should be played at in order to seem as if it is a recording of that note.
static double GetNoteSampleRate(int note, int instrument_pitch)
{
	return ((instrument_pitch - 1000.0)/100.0 + pitch[note])*44100/1550;
}

// converts a time in milliseconds to that same time length in samples
static int MSToSamples(int ms)
{
	return (int)(((double)SAMPLE_RATE / (double)1000) * (double)ms);
}

// converts a sample length to milliseconds
//static int SamplesToMS(int samples)
//{
//	return (int)(((double)samples * 1000) / SAMPLE_RATE);
//}

#ifndef DRUM_PXT

static bool load_drum(char *fname, int d)
{
Mix_Chunk *chunk;
int i, read_pt;
int left,right;
signed short *abuf;

	//stat("load_drum: loading %s into drum index %d", fname, d);
	if (!(chunk = Mix_LoadWAV(fname)))
	{
		staterr("Missing drum sample: '%s'", fname);
		return 1;
	}
	
	//stat("chunk: %d bytes in chunk", chunk->alen);
	drumtable[d].nsamples = chunk->alen / 2 / 2;	// 16-bit stereo sound
	drumtable[d].samples = malloc(drumtable[d].nsamples * 2);
	
	#ifndef QUIET
		stat("drum0%X [%s]: %d samples", d, fname, drumtable[d].nsamples);
	#endif
	
	read_pt = 0;
	abuf = (signed short *)chunk->abuf;
	for(i=0;i<drumtable[d].nsamples;i++)
	{
		left = abuf[read_pt++]; right = abuf[read_pt++];
		
		drumtable[d].samples[i] = (left + right) / 2;
		drumtable[d].samples[i] += drumtable[d].samples[i];		// make drums louder--sounds better
	}
	
	Mix_FreeChunk(chunk);
	return 0;
}

#else

static bool load_drum_pxt(const std::string& fname, int d)
{
int i;
signed short sample;
stPXSound snd;

	stat("load_drum: loading %s into drum index %d", fname.c_str(), d);
	
	if (pxt_load(fname, &snd)) return 1;
	pxt_Render(&snd);
	
	drumtable[d].nsamples = snd.final_size;
	drumtable[d].samples = (signed short *)malloc(snd.final_size * 2);		// *2 - it is 16-bit
	
	#ifndef QUIET
		stat("drum0%X [%s]: %d samples", d, fname.c_str(), drumtable[d].nsamples);
	#endif
	
	// read data out of pxt's render result and put it into our drum sample table
	for(i=0;i<drumtable[d].nsamples;i++)
	{
		sample = snd.final_buffer[i];
		//i'm upscaling the 8-bit value to 16-bit;
		//but this also sets volume of drums relative to music
		sample *= 200;
		
		drumtable[d].samples[i] = sample;
	}
	
	FreePXTBuf(&snd);
	return 0;
}

#endif



static bool load_drumtable(const std::string& pxt_path)		// pxt_path = the path where drum pxt files can be found
{
char fname[80];
int d;

	#ifndef DRUM_PXT
		for(d=0;d<NUM_DRUMS;d++)
		{
			sprintf(fname, "./drums/%s.wav", drum_names[d]);
			if (load_drum(fname, d)) return 1;
		}
	#else
		
		
		stat("load_drumtable: rebuilding drums...");
		
		pxt_initsynth();
		
		for(d=0;d<NUM_DRUMS;d++)
		{
			if (drum_pxt[d])
			{
				sprintf(fname, "%sfx%02x.pxt", pxt_path.c_str(), drum_pxt[d]);
				if (load_drum_pxt(fname, d)) return 1;
			}
		}
		
	#endif
	
	//for(d=0;d<256;d++) { lprintf("%d ", drumtable[0].samples[d]); if (d%32==0) lprintf("\n"); }
	//lprintf("\n");
	
	return 0;
}




static bool load_wavetable(const std::string& fname)
{
int wav, sampl;
FILE *fp;
#define BUF_SIZE		(100 * 256)
signed char buffer[BUF_SIZE + 1];
signed char *ptr;

	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp)
	{
		stat("Unable to open wavetable.dat!!");
		return 1;
	}
	
	fread(buffer, BUF_SIZE, 1, fp);
	fclose(fp);
	
	ptr = &buffer[0];
	for(wav=0;wav<100;wav++)
	{
		for(sampl=0;sampl<256;sampl++)
		{
			// 256 = (32768 / 128)-- convert to 16-bit
			wavetable[wav].samples[sampl] = (signed short)((int)(*ptr++) << 8);
		}
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/


int org_init(const std::string& wavetable_fname, const std::string& drum_pxt_dir, int org_volume)
{
int i;
	
	//SSReserveChannel(ORG_CHANNEL);
	OrgVolume = org_volume;
	
	// set all buffer pointers and things to NULL, so if something fails to load,
	// we won't crash on org_close.
	memset(drumtable, 0, sizeof(drumtable));
	for(i=0;i<16;i++) note_channel[i].outbuffer = NULL;
	final_buffer.samples = NULL;
	
	init_pitch();
	if (load_wavetable(wavetable_fname)) return 1;
	if (load_drumtable(drum_pxt_dir)) return 1;
	
	song.playing = false;
	org_inited = true;
	return 0;
}

static void free_buffers(void)
{
int i;

	for(i=0;i<16;i++)
		if (note_channel[i].outbuffer) free(note_channel[i].outbuffer);
	
	if (final_buffer.samples) free(final_buffer.samples);
}

void org_close(void)
{
int d;

	org_stop();
	free_buffers();
	
	for(d=0;d<NUM_DRUMS;d++)
		if (drumtable[d].samples) free(drumtable[d].samples);
}

static bool init_buffers(void)
{
int i;

	// free the old buffers, as we're probably going to change their size here in a sec
	free_buffers();
	
	/* figure some stuff out real quick about buffer lengths --- */
	
	// convert the ms-per-beat stuff into samples
	song.samples_per_beat = MSToSamples(song.ms_per_beat);
	song.note_closing_samples = MSToSamples(song.ms_of_last_beat_of_note);
	// take the suggestion on cache ahead time (which is in ms) and figure out how many beats that is
	buffer_beats = (cache_ahead_time / song.ms_per_beat) + 1;
	if (buffer_beats < 3) buffer_beats = 3;
	
	// now figure out how many samples that is.
	buffer_samples = (buffer_beats * song.samples_per_beat);
	// now figure out how many bytes THAT is.
	outbuffer_size_bytes = buffer_samples * 2 * 2;		// @ 16-bits, and stereo sound
	
	
	// initialize the per-channel output buffers
	for(i=0;i<16;i++)
	{
		note_channel[i].outbuffer = (signed short *)malloc(outbuffer_size_bytes);
		note_channel[i].number = i;
		//memset(note_channel[i].outbuffer, 0, outbuffer_size_bytes);
	}
	
	// initialize the final (mixed) output buffers
	final_buffer.samples = (signed short *)malloc(outbuffer_size_bytes);
	memset(final_buffer.samples, 0, outbuffer_size_bytes);
	
	return 0;
}


char org_load(const std::string& fname)
{
static const char *magic = "Org-02";
char buf[8];
FILE *fp;
int i, j;

	fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
	if (!fp) { stat("org_load: no such file: '%s'", fname.c_str()); return 1; }
	
	for(i=0;i<6;i++) { buf[i] = fgetc(fp); } buf[i] = 0;
	if (strcmp(buf, magic)) { stat("org-load: not an org file (got '%s')", buf); fclose(fp); return 1; }
	stat("%s: %s detected", fname.c_str(), magic);
	
	fseek(fp, 0x06, SEEK_SET);
	
	song.last_pos=0;
	song.ms_per_beat = fgeti(fp);
	song.steps_per_bar = fgetc(fp);
	song.beats_per_step = fgetc(fp);
	song.loop_start = fgetl(fp);
	song.loop_end = fgetl(fp);
	
	//song.ms_per_beat = 500;
	//song.loop_start = 64;
	
	if (song.loop_end < song.loop_start)
	{
		stat("org_load: loop end is before loop start");
		fclose(fp);
		return 1;
	}
	
	// compute how long the last beat of a note should be (it should not use up the whole beat)
	song.ms_of_last_beat_of_note = song.ms_per_beat - (int)((double)song.ms_per_beat * 0.1);
	
	// not actually used in this module, but the larger program might want to know this
	song.beats_per_bar = (song.beats_per_step * song.steps_per_bar);
	
	/*lprintf("tempo: %d ms/beat\n", song.ms_per_beat);
	lprintf("beats_per_step: %d\n", song.beats_per_step);
	lprintf("steps_per_bar: %d\n", song.steps_per_bar);
	lprintf("loop begins on beat %d\n", song.loop_start);
	lprintf("loop ends on beat %d\n", song.loop_end);*/
	
	for(i=0;i<16;i++)
	{
		song.instrument[i].pitch = fgeti(fp);
		song.instrument[i].wave = fgetc(fp);
		song.instrument[i].pi = fgetc(fp);
		song.instrument[i].nnotes = fgeti(fp);
		
		if (song.instrument[i].nnotes >= MAX_SONG_LENGTH)
		{
			stat(" * org_load: instrument %d has too many notes! (has %d, max %d)", i, song.instrument[i].nnotes, MAX_SONG_LENGTH);
			fclose(fp);
			return 1;
		}
		
		/*if (song.instrument[i].nnotes)
		{
			lprintf("Instrument %d: ", i);
			lprintf(" Pitch: %d, ", song.instrument[i].pitch);
			lprintf(" Wave: %d, ", song.instrument[i].wave);
			lprintf(" Pi: %d, ", song.instrument[i].pi);
			lprintf(" Nnotes: %d\n", song.instrument[i].nnotes);
		}*/
		
		// substitute unavailable drums
		// credits track for one, has Per02 set which CS didn't actually have, I don't think
		if (i >= 8)
		{
			switch(song.instrument[i].wave)
			{
				case 9: song.instrument[i].wave = 8; break;
			}
		}
	}
	
	for(i=0;i<16;i++)
	{
		for(j=0;j<song.instrument[i].nnotes;j++) song.instrument[i].note[j].beat = fgetl(fp);
		for(j=0;j<song.instrument[i].nnotes;j++) song.instrument[i].note[j].note = fgetc(fp);
		for(j=0;j<song.instrument[i].nnotes;j++) song.instrument[i].note[j].length = fgetc(fp);
		for(j=0;j<song.instrument[i].nnotes;j++) song.instrument[i].note[j].volume = fgetc(fp);
		for(j=0;j<song.instrument[i].nnotes;j++) song.instrument[i].note[j].panning = fgetc(fp);
	}
	
	fclose(fp);
	return init_buffers();
}

// adds num_samples samples of silence to the output buffer of channel "m".
static void silence_gen(stNoteChannel *chan, int num_samples)
{
int clear_bytes;

	//stat("silence_gen: making %d samples of silence", num_samples);
	
	clear_bytes = (num_samples * 2 * 2);		// clear twice as many shorts as = num_samples
	memset(&chan->outbuffer[chan->outpos], 0, clear_bytes);
	
	chan->samples_so_far += num_samples;
	chan->outpos += (num_samples * 2);
}


// ensures that there are exactly desired_samples contained in the output buffer of instrument m.
// if there are fewer samples than desired, the gap is filled with silence.
// if there are more, the extra audio is truncated.
static void ForceSamplePos(int m, int desired_samples)
{
	if (note_channel[m].samples_so_far != desired_samples)
	{
		if (desired_samples > note_channel[m].samples_so_far)
		{
			silence_gen(&note_channel[m], (desired_samples - note_channel[m].samples_so_far));
		}
		else
		{	// this should NEVER actually happen!!
			stat("ForceSamplePos: WARNING: !!! truncated channel %d from %d to %d samples !!!", m, note_channel[m].samples_so_far, desired_samples);
			note_channel[m].samples_so_far = desired_samples;
			note_channel[m].outpos = desired_samples * 2;
		}
	}
}

// combines all of the individual channel output buffers into a single, final, buffer.
static void mix_buffers(void)
{
int i, cursample, len;
int mixed_sample;
signed short *final;

//	lprintf("mix_buffers: mixing channels into final_buffer[%d]\n", current_buffer);
	
	// go up to samples*2 because we're mixing the stereo audio output from calls to WAV_Synth
	len = buffer_samples * 2;
	final = final_buffer.samples;
	
	//stat("mixing %d samples", len);
	for(cursample=0;cursample<len;cursample++)
	{
		// first mix instruments
		mixed_sample = note_channel[0].outbuffer[cursample];
		for(i=1;i<16;i++) mixed_sample += note_channel[i].outbuffer[cursample];
		
		if (mixed_sample > 32767) mixed_sample = 32767;
		else if (mixed_sample < -32768) mixed_sample = -32768;
		
		final[cursample] = SDL_SwapLE16(mixed_sample);
	}
}


// -------------------
// note_open
// -------------------
// initializes the synthesis of a new note.
// chan: the instrument channel the note will play on
// wave: the instrument no to play the note with
// pitch: the pitch variation of the instrument as set in the org
// note: the note # we'll be playing
// total_ms: the maximum length the note will play for (controls buffer allocation length)
static void note_open(stNoteChannel *chan, int wave, int pitch, int note)
{
double new_sample_rate;
#define	samplK	 	 11025		// constant is original sampling rate of the samples in the wavetable

	// compute how quickly, or slowly, to play back the wavetable sample
	new_sample_rate = GetNoteSampleRate(note, pitch);
	chan->sample_inc = (new_sample_rate / (double)samplK);
	
	chan->wave = wave;
	chan->phaseacc = 0;
	
	//lprintf("note_open: new note opened for channel %08x at sample_inc %.2f, using wave %d\n", chan, chan->sample_inc, chan->wave);
}

// given a volume and a panning value, it returns three values
// between 0 and 1.00 which are how much to scale:
//	the whole sound (volume_ratio)
//  just the left channel (volume_left_ratio)
//  just the right channel (volume_right_ratio)
static void ComputeVolumeRatios(int volume, int panning, double *volume_ratio, \
								double *volume_left_ratio, double *volume_right_ratio)
{
	*volume_ratio = ((double)volume / ORG_MAX_VOLUME);
	
	// get volume ratios for left and right channels (panning)
	if (panning < ORG_PAN_CENTERED)
	{	// panning left (make right channel quieter)
		*volume_right_ratio = ((double)panning / ORG_PAN_CENTERED);
		*volume_left_ratio = 1.00f;
	}
	else if (panning > ORG_PAN_CENTERED)
	{	// panning right (make left channel quieter)
		*volume_left_ratio = ((double)(ORG_PAN_FULL_RIGHT - panning) / ORG_PAN_CENTERED);
		*volume_right_ratio = 1.00f;
	}
	else
	{	// perfectly centered (both channels get the full volume)
		*volume_left_ratio = 1.00f;
		*volume_right_ratio = 1.00f;
	}
}


// Interpolates a new sample from two samples which will be "in-between" the two samples.
// if ratio is 0.00, it will return exactly sample1.
// if ratio is 1.00, it will return exactly sample2.
// and if ratio is something like 0.5, it will mix the samples together.
static double Interpolate(int sample1, int sample2, double ratio)
{
double s1, s2;
	s1 = ((double)sample1 * (1.00f - ratio));
	s2 = ((double)sample2 * ratio);
	return (s1 + s2);
}


// -------------------
// note_gen
// -------------------
// Adds num_samples worth of audio data to the channel at the current frequency, volume,
// panning, and pitch settings, and at the note & wave spec'd in note_open.
static void note_gen(stNoteChannel *chan, int num_samples)
{
int i;
double audioval;
double master_volume_ratio, volume_left_ratio, volume_right_ratio;
int wave;
unsigned char pos1, pos2;
double iratio;

	wave = chan->wave;
	
	// compute volume ratios; unlike drums we have to do this every time
	// since they can change in the middle of the note.
	ComputeVolumeRatios(chan->volume, chan->panning,
				&master_volume_ratio, &volume_left_ratio, &volume_right_ratio);
	
	//statbuild("Entering note_gen with phaseacc=%.2f and sample_inc=%.2f", chan->phaseacc, chan->sample_inc);
	//statbuild(", using buffer %08x\n", chan->outbuffer);
	
	//stat("note_gen(%d, %d)", chan->number, num_samples);
	
	// generate however many output samples we were asked for
	for(i=0;i<num_samples;i++)
	{
		// interpolate a sample that's at the fractional "phaseacc" sample position in the wavetable form
		pos1 = (int)chan->phaseacc;
		pos2 = pos1 + 1;		// since pos1&2 are chars, this wraps at 255
		iratio = chan->phaseacc - (int)chan->phaseacc;
		
		audioval = Interpolate(wavetable[wave].samples[pos1], wavetable[wave].samples[pos2], iratio);
		audioval *= master_volume_ratio;
		
		chan->outbuffer[chan->outpos++] = (int)(audioval * volume_left_ratio);
		chan->outbuffer[chan->outpos++] = (int)(audioval * volume_right_ratio);
		chan->samples_so_far++;
		
		chan->phaseacc += chan->sample_inc;
		if ((int)chan->phaseacc >= 256) chan->phaseacc -= 256;
	}
}


// -------------------
// note_close
// -------------------
// ends a note smoothly by ensuring that it's wave stops near the 0-crossing point.
// this avoids a slight popping noise which can be caused by abruptly moving from
// a high sample value to a close-to-zero sample value.
// returns the # of extra samples generated.
static int note_close(stNoteChannel *chan)
{
	if (chan->outpos == 0)
		return 0;
	
	int samples_made = 0;
	while(chan->samples_so_far < buffer_samples)	// avoid potential buffer overflow
	{
		// get the value of the last sample in the buffer and check
		// if it's close enough to silence yet. if not, let the note
		// run on a teeny bit longer than it's supposed to until it's wave
		// hits the zero-crossing point, to avoid a click.
		int last_sample = chan->outbuffer[chan->outpos - 1];
		if (abs(last_sample) < 1000) break;
		
		note_gen(chan, 1);
		samples_made++;
	}
	
	return samples_made;
}


// set up to make a drum noise using drum "wave" at note "note" on channel "m_channel".
// the total number of samples the drum will last is returned.
static int drum_open(int m_channel, int wave, int note)
{
stNoteChannel *chan = &note_channel[m_channel];
double new_sample_rate;
int gen_samples;

	//lprintf("drum_hit: playing drum %d[%s] on channel %d, note %02x volume %d panning %d\n", wave, drum_names[wave], m_channel, note, chan->volume, chan->panning);
	
	new_sample_rate = GetNoteSampleRate(note, song.instrument[m_channel].pitch);
	chan->sample_inc = (new_sample_rate / (double)drumK);
	
	// get the new number of samples for the sound
	gen_samples = (int)((double)drumtable[wave].nsamples / chan->sample_inc);
	
	// precompute volume and panning values since they're the same over the length of the drum
	ComputeVolumeRatios(chan->volume, chan->panning, &chan->master_volume_ratio, &chan->volume_left_ratio, &chan->volume_right_ratio);
	
	chan->wave = wave;
	chan->phaseacc = 0;
	return gen_samples;
}


// generates "num_samples" of a drum noise previously set up via drum_open into the output
// buffer of the channel.
static void drum_gen(int m_channel, int num_samples)
{
stNoteChannel *chan = &note_channel[m_channel];
double volume_ratio, volume_left_ratio, volume_right_ratio;
int wave;
int pos1, pos2;
double iratio;
double audioval;
int i;

	volume_ratio = chan->master_volume_ratio;
	volume_left_ratio = chan->volume_left_ratio;
	volume_right_ratio = chan->volume_right_ratio;
	wave = chan->wave;
	
	//stat("drum_gen(%d, %d)", m_channel, num_samples);
	
	// generate the drum sound
	for(i=0;i<num_samples;i++)
	{
		pos1 = (int)chan->phaseacc;
		pos2 = pos1 + 1;
		if (pos2 >= drumtable[wave].nsamples) pos2 = pos1;
		
		iratio = chan->phaseacc - (int)chan->phaseacc;
		
		audioval = Interpolate(drumtable[wave].samples[pos1], drumtable[wave].samples[pos2], iratio);
		audioval *= volume_ratio;
		
		chan->outbuffer[chan->outpos++] = (signed short)(audioval * volume_left_ratio);
		chan->outbuffer[chan->outpos++] = (signed short)(audioval * volume_right_ratio);
		chan->samples_so_far++;
		
		chan->phaseacc += chan->sample_inc;
		if ((int)chan->phaseacc > drumtable[wave].nsamples)
		{
			staterr(" **ERROR-phaseacc ran over end of drumsample %.2f %d", chan->phaseacc, drumtable[wave].nsamples);
			break;
		}
	}
}


// generate up to a 1 beat worth of music from channel "m" at the song.beat cursor point.
// it may generate less.
static void NextBeat(int m)
{
stNoteChannel *chan = &note_channel[m];
stInstrument *track = &song.instrument[m];
//int volume, panning;
stNote *note;
int len;

	// add notes as long as instrument has notes left to add
	if (track->curnote < track->nnotes)
	{
		for(;;)
		{
			// when we hit the loop start point, record the note we were on for later
			if (song.beat == song.loop_start)
			{
				track->loop_note = track->curnote;
			}
			
			// get a pointer to the note at current position in the song
			note = &track->note[track->curnote];
			
			// skip ahead if the song got ahead of us somehow
			if (song.beat > note->beat)
			{
				if (++track->curnote >= track->nnotes)
					return;
			}
			else break;
		}
		
		// 1st- start notes as we arrive at their beat
		if (song.beat == note->beat)
		{
			//stat(" Beat/Note: %d/%d   Chan: %d   Note: %d  length=%d vol=%d pan=%d wave=%d", song.beat, curnote, m, note->note, note->length, note->volume, note->panning, song.instrument[m].wave);
			
			if (note->volume != 0xff) chan->volume = note->volume;
			if (note->panning != 0xff) chan->panning = note->panning;
			
			if (note->note != 0xff)
			{
				if (m < 8)
				{
					note_open(chan, track->wave, track->pitch, note->note);
					chan->length = note->length;
				}
				else
				{	// on percussion tracks the length works differently---drum_open returns the
					// number of samples the drum will take to finish playing.
					chan->length = drum_open(m, track->wave, note->note);
				}
			}
			
			track->curnote++;
		}
	}
	
	// 2nd- generate any notes which are running
	if (chan->length)
	{
		if (m < 8)
		{	// melody tracks
			if (track->pi)
			{	// pi tracks always generate only 1024 samples for ANY note
				note_gen(chan, 1024);
				chan->length = 0;
			}
			else
			{
				if (chan->length > 1)
				{	// generate a full beat of music
					note_gen(chan, song.samples_per_beat);
				}
				else	// generate only most of the beat--if there's a note immediately after
				{		// this one they should not run together
					note_gen(chan, song.note_closing_samples);
				}
				
				if (!--chan->length)
				{
					note_close(chan);
				}
			}
		}
		else
		{	// percussion tracks
			// if less than a whole beats worth of samples is left to play on the drum, finish
			// whatever's left. Else generate only one beats worth right now.
			if (chan->length > song.samples_per_beat)
				len = song.samples_per_beat;
			else
				len = chan->length;
			
			drum_gen(m, len);
			
			chan->length -= len;
		}
	}

}


// generate a buffer's worth of music and place it in the current final buffer.
static void generate_music(void)
{
int m;
int beats_left;
int out_position;

	//stat("generate_music: cb=%d buffer_beats=%d", current_buffer, buffer_beats);
	
	// save beat # of the first beat in buffer for calculating current beat for TrackFuncs
	final_buffer.firstbeat = song.beat;
	song.last_gen_beat = song.beat;
	song.last_gen_tick = SDL_GetTicks();
	
	// clear all the channel buffers
	for(m=0;m<16;m++)
	{
		note_channel[m].samples_so_far = 0;
		note_channel[m].outpos = 0;
	}
	
	//stat("generate_music: generating %d beats of music\n", buffer_beats);
	beats_left = buffer_beats;
	out_position = 0;
	
	while(beats_left)
	{
		out_position += song.samples_per_beat;
		
		// for each channel...
		for(m=0;m<16;m++)
		{
			// generate any music that's supposed to go into the current beat
			NextBeat(m);
			// ensure that exactly one beat of samples was added to the channel by inserting silence
			// if needed. sometimes NextBeat may not actually generate a full beats worth, for
			// example if there was no note playing on the track, of if it was the last beat of a note.
			ForceSamplePos(m, out_position);
		}
		
		if (++song.beat >= song.loop_end)
		{
			//Reset play time, taking into account loop start
			song.beat = song.loop_start;
			song.haslooped = true;
			
			for(m=0;m<16;m++)
			{
				song.instrument[m].curnote = song.instrument[m].loop_note;
				note_channel[m].length = 0;
			}
		}
		
		beats_left--;
	}
	
	mix_buffers();
}


void orgMusicPlayer(void *udata, Uint8 *stream, int len)
{
	SDL_AudioFormat format;
	int frequency, channels;

	Mix_QuerySpec(&frequency, &format, &channels);

//    stSong* song  = (stSong*)udata;

	SDL_memset(stream, 0, len);
	Uint8* tmp = (Uint8*)malloc(len);

	Uint8 *ptr = (Uint8*)final_buffer.samples;
	int idx = song.last_pos;
	for (int i =0; i < len; i++)
	{
		if (idx>=buffer_samples*4)
		{
			generate_music();
			idx=0;
		}
		
		tmp[i] = ptr[idx];
		idx++;
	}
	song.last_pos = idx;
	
	SDL_MixAudioFormat(stream, tmp, format, len, song.volume);
	free(tmp);
}

// start the currently-loaded track playing at beat startbeat.
bool org_start(int startbeat)
{
    printf("startbeat: %d\n", startbeat);
	org_stop();		// stop any old music
	
	// set all the note-tracking stuff to starting values
	song.beat = startbeat;
	song.haslooped = false;
	song.last_pos = 0;
	
	for(int i=0;i<16;i++)
	{
		song.instrument[i].curnote = 0;
		note_channel[i].volume = ORG_MAX_VOLUME;
		note_channel[i].panning = ORG_PAN_CENTERED;
		note_channel[i].length = 0;
	}
	
	// fill the first buffer and play it to jumpstart the playback cycle
	//lprintf(" ** org_start: Jumpstarting buffer cycle\n");
	
	song.playing = true;
	song.fading = false;
	
	song.volume = OrgVolume;
	
	// kickstart the first buffer
	generate_music();
	Mix_HookMusic(orgMusicPlayer, &song);
	
	return 0;
}


// pause/stop playback of the current song
void org_stop(void)
{
	extern int lastsongpos;
	if (song.playing)
	{
		/* Okay, this is hackish, and still misses a beat or two.
		   Sadly, there's no better way on SDL, because it writes
		   to audio device in bulk and there's no way of knowing
		   how many samples actually played.
		*/
		uint32_t delta = SDL_GetTicks() - song.last_gen_tick;
		uint32_t beats = (double)delta / (double)song.ms_per_beat;
		uint32_t cur_beat = song.last_gen_beat + beats;
		if (cur_beat >= song.loop_end)
		{
		    cur_beat = song.loop_start + (cur_beat - song.loop_end);
		}
		lastsongpos = cur_beat;
		song.playing = false;
		Mix_HookMusic(NULL,NULL);
	}
}

bool org_is_playing(void)
{
	return song.playing;
}

void org_fade(void)
{
	stat("org_fade");
	song.fading = true;
	song.last_fade_time = 0;
}

void org_set_volume(int newvolume)
{
	if (newvolume != song.volume)
	{
		song.volume = newvolume;
		//SSSetVolume(ORG_CHANNEL, newvolume);
	}
}

void org_run_fade()
{
	if (!song.fading) return;
	uint32_t curtime = SDL_GetTicks();
	if ((curtime - song.last_fade_time) >= 25)
	{
		int newvol = (song.volume - 1);
		if (newvol <= 0)
		{
			song.fading = false;
			org_stop();
		}
		else
		{
			org_set_volume(newvol);
		}
		
		song.last_fade_time = curtime;
	}
}

