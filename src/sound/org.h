
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
	uint32_t beat;			// beat no. that note starts on
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
	
	uint32_t loop_start, loop_end;
	
	stInstrument instrument[16];
	
	uint32_t beat;
	char haslooped;
	
	bool fading;
	uint32_t last_fade_time;
	uint32_t last_pos;
	uint32_t last_gen_tick;
	uint32_t last_gen_beat;
};

int org_init(const std::string& wavetable_fname, const std::string& drum_pxt_dir, int org_volume);
void org_close(void);
char org_load(const std::string& fname);
bool org_start(int startbeat);
void org_stop(void);
bool org_is_playing(void);
void org_fade(void);
void org_set_volume(int newvolume);
void org_run_fade(void);

#endif

