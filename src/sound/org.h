
#ifndef _ORG_H
#define _ORG_H

// SSChannel # to play the music on
#define ORG_CHANNEL			15

// maximum possible volume of an org instrument (in the file)
// i hear according to orgmaker you can't go higher than F8 but a few
// songs actually do go all the way to FF.
#define ORG_MAX_VOLUME		255

// which panning value is perfectly centered
#define ORG_PAN_CENTERED	6
#define ORG_PAN_FULL_RIGHT	(ORG_PAN_CENTERED + ORG_PAN_CENTERED)

#define KEYS_OCTAVE			12
#define NUM_NOTES			(8 * KEYS_OCTAVE)	// range of note values: 00-this.
#define NUM_DRUMS			12					// max # of drum instruments to load

#define MAX_SONG_LENGTH		5000		// max song length to allocate for, in notes


// this handles the actual synthesis
struct stNoteChannel
{
	signed short *outbuffer;
	
	// position inside outbuffer (not the same as samples_so_far because org module outputs stereo.
	// outpos counts data, samples_so_far counts samples. samples_so_far=outpos*2)
	int outpos;				// incs by 2 for each samples_so_far, one for left ch one for right ch
	int samples_so_far;		// number of samples generated so far into outbuffer
	
	double phaseacc;		// current read position inside wavetable sample
	double sample_inc;		// speed at which to iterate over the wavetable waveform
	
	// for drums
	double master_volume_ratio, volume_left_ratio, volume_right_ratio;
	
	int wave;				// index into wavetable (which instrument we're using)
	int volume;				// last volume value sent to note_gen
	int panning;			// last panning value sent to note_gen
	int length;				// # of beats of the current note left to generate
	
	int number;				// the chanel number of this channel
};


struct stNote
{
	int beat;			// beat no. that note starts on
	uint8_t note;			// 00 - 5F, starts on a C
	uint8_t length;		// in beats
	uint8_t volume;		// 00 - F8
	uint8_t panning;		// 00 - 0C
};

// keeps track of instrument settings for a track
struct stInstrument
{
	int pitch;
	int wave;				// which wave (00-99) to use
	// if pi is set all notes on the channel play for 1024 samples regardless
	// of length or tempo settings. pi only has meaning on the instrument tracks.
	bool pi;
	
	int curnote;			// current note (during playback)
	int loop_note;			// used when looping back to the beginning of a song at beat loop_end
	
	int nnotes;
	stNote note[MAX_SONG_LENGTH];
};

struct stSong
{
	bool playing;
	int volume;
	
	int ms_per_beat, ms_of_last_beat_of_note;
	int beats_per_step;
	int steps_per_bar;
	int beats_per_bar;				// == (beats_per_step * steps_per_bar)
	
	int samples_per_beat;			// # of samples in each beat
	int note_closing_samples;		// # of samples of note to generate at the last beat of a note
	
	int loop_start, loop_end;
	
	stInstrument instrument[16];
	
	int beat;
	char haslooped;
	
	bool fading;
	uint32_t last_fade_time;
};

static void init_pitch(void);
static double GetNoteSampleRate(int note, int instrument_pitch);
static int MSToSamples(int ms);
static int SamplesToMS(int samples);
static bool load_drumtable(const char *pxt_path);
static bool load_drum(char *fname, int d);
static bool load_drum_pxt(char *fname, int d);
static bool load_wavetable(const char *fname);
int org_init(const char *wavetable_fname, const char *drum_pxt_dir, int org_volume);
void org_close(void);
char org_load(char *fname);
static bool init_buffers(void);
static void free_buffers(void);
bool org_start(int startbeat);
void org_stop(void);
bool org_is_playing(void);
void org_fade(void);
void org_set_volume(int newvolume);
static void runfade();
static void mix_buffers(void);
static void queue_final_buffer(void);
static void OrgBufferFinished(int channel, int buffer_no);
static void ComputeVolumeRatios(int volume, int panning, double *volume_ratio, double *volume_left_ratio, double *volume_right_ratio);
static double Interpolate(int sample1, int sample2, double ratio);
static void ForceSamplePos(int m, int desired_samples);
static void silence_gen(stNoteChannel *chan, int num_samples);
static void note_open(stNoteChannel *chan, int wave, int pitch, int note);
static void note_gen(stNoteChannel *chan, int num_samples);
static int note_close(stNoteChannel *chan);
static int drum_open(int m_channel, int wave, int note);
static void drum_gen(int m_channel, int num_samples);
void org_run(void);
static void generate_music(void);
static void NextBeat(int m);

#endif

