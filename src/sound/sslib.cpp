
// Sound System
// more or less, my own version of SDL_mixer

#include <SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../common/basics.h"

#include "sslib.h"

SSChannel channel[SS_NUM_CHANNELS];
SDL_AudioSpec spec;

uint8_t *mixbuffer = NULL;
int mix_pos;

int lockcount = 0;


// add the contents of the chunk at head to the mix_buffer.
// don't add more than bytes.
// return the number of bytes that were added.
static int AddBuffer(SSChannel *chan, int bytes)
{
	SSChunk *chunk = &chan->chunks[chan->head];
	
	if (bytes > chunk->bytelength)
	{
		bytes = chunk->bytelength;
	}
	
	// don't copy past end of chunk
	if (chunk->bytepos+bytes > chunk->bytelength)
	{
		// add it to list of finished chunks
		chan->FinishedChunkUserdata[chan->nFinishedChunks++] = chunk->userdata;
		
		// only add what's left. and advance the head pointer to the next chunk.
		bytes = chunk->bytelength - chunk->bytepos;
		if (++chan->head >= MAX_QUEUED_CHUNKS) chan->head = 0;
		
		//stat("AddBuffer: reached end of chunk %d; new head is %d, and tail is %d", c, chan->head, chan->tail);
	}
	
//	stat("%d: Channel %d: Copying %d bytes from chunk %d @ %08x -- pos=%d, len=%d", SDL_GetTicks(), cnn, bytes, c, chunk->bytebuffer, chunk->bytepos, chunk->bytelength);
	memcpy(&mixbuffer[mix_pos], &chunk->bytebuffer[chunk->bytepos], bytes);
	mix_pos += bytes;
	chunk->bytepos += bytes;
	
	return bytes;
}


static void mixaudio(void *unused, uint8_t *stream, int len)
{
int bytes_copied;
int bytestogo;
int c;
int i;

#if SDL_VERSION_ATLEAST(1, 3, 0)
	/* Need to initialize the stream in SDL 1.3+ */
	memset(stream, spec.silence, len);
#endif

	// get data for all channels and add it to the mix
	for(c=0;c<SS_NUM_CHANNELS;c++)
	{
		if (channel[c].head==channel[c].tail) continue;
		
		bytestogo = len;
		mix_pos = 0;
		while(bytestogo > 0)
		{
			bytes_copied = AddBuffer(&channel[c], bytestogo);
			bytestogo -= bytes_copied;
			
			if (channel[c].head==channel[c].tail)
			{		// ran out of chunks before buffer full
				// clear remaining portion of mixbuffer
				if (bytestogo)
				{
					memset(&mixbuffer[mix_pos], spec.silence, bytestogo);
				}
				
				break;
			}
		}
		
		SDL_MixAudio(stream, mixbuffer, len, channel[c].volume);
	}
	
	// tell any callbacks that had a chunk finish, that their chunk finished
	for(c=0;c<SS_NUM_CHANNELS;c++)
	{
		if (channel[c].FinishedCB)
		{
			for(i=0;i<channel[c].nFinishedChunks;i++)
			{
				//stat("Telling channel %d's handler that chunk %d finished", c, channel[c].FinishedChunkUserdata[i]);
				(*channel[c].FinishedCB)(c, channel[c].FinishedChunkUserdata[i]);
			}
		}
		
		channel[c].nFinishedChunks = 0;
	}
}


char SSInit(void)
{
SDL_AudioSpec fmt;

	// Set 16-bit stereo audio at 22Khz
	fmt.freq = SAMPLE_RATE;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 512;
	fmt.callback = mixaudio;
	fmt.userdata = NULL;
	
	// Open the audio device and start playing sound!
	if (SDL_OpenAudio(&fmt, &spec) < 0)
	{
		staterr("SS: Unable to open audio: %s", SDL_GetError());
		return 1;
	}
	
	if (spec.format != fmt.format || \
		spec.channels != fmt.channels)
	{
		staterr("SS: Failed to obtain the audio format I wanted");
		return 1;
	}
	
	mixbuffer = (uint8_t *)malloc(spec.samples * spec.channels * 2);
	
	// zero everything in all channels
	memset(channel, 0, sizeof(channel));
	for(int i=0;i<SS_NUM_CHANNELS;i++)
		channel[i].volume = SDL_MIX_MAXVOLUME;
	
	stat("sslib: initilization was successful.");
	
	lockcount = 0;
	SDL_PauseAudio(0);
	return 0;
}

void SSClose(void)
{
	SDL_CloseAudio();
	if (mixbuffer) free(mixbuffer);
}

/*
void c------------------------------() {}
*/

// reserves a channel so that it will not be returned by SSFindFreeChannel and no
// sounds will be allocated to it by SSEnqueueChunk with a -1 parameter.
// thus, the channel can only be played on by explicitly playing a sound to it.
void SSReserveChannel(int c)
{
	channel[c].reserved = 1;
}

// returns the first available channel that is not playing a sound.
// if all chans are full, returns -1.
int SSFindFreeChannel(void)
{
int i;
	for(i=0;i<SS_NUM_CHANNELS;i++)
	{
		if (channel[i].head==channel[i].tail && !channel[i].reserved) return i;
	}
	return -1;
}

// enqueue a chunk of sound to a channel.
// c:			channel to play on, or pass -1 to automatically find a free one
// buffer:		16-bit S16 22050Hz audio data to play
// len:			buffer length in stereo samples. len=1 means 4 bytes: *2 for 16-bit, and *2 for stereo.
// userdata:	a bit of application-defined data to associate with the chunk,
// 				such as a game sound ID. this value will be passed to the FinishedCallback()
//				when the chunk completes.
// FinishedCB:	an optional callback function to call when the chunk stops playing.
//
// returns:		the channel sound was started on, or -1 if failure.
int SSEnqueueChunk(int c, signed short *buffer, int len, int userdata, void(*FinishedCB)(int, int))
{
SSChannel *chan;
SSChunk *chunk;

	if (c >= SS_NUM_CHANNELS)
	{
		staterr("SSEnqueueChunk: channel %d is higher than SS_NUM_CHANNELS", c);
		return -1;
	}
	
	SSLockAudio();
	
	if (c < 0) c = SSFindFreeChannel();
	if (c==-1)
	{
		staterr("SSEnqueueChunk: no available sound channels!");
		SSUnlockAudio();
		return -1;
	}
	
	chan = &channel[c];
	
	chan->FinishedCB = FinishedCB;
	
	chunk = &chan->chunks[chan->tail];
	chunk->buffer = buffer;
	chunk->length = len;							// in 16-bit stereo samples
	chunk->userdata = userdata;
	
	chunk->bytebuffer = (signed char *)buffer;
	chunk->bytelength = chunk->length * 2 * 2;		// in bytes
	
	chunk->bytepos = 0;
	
	// advance tail pointer
	if (++chan->tail >= MAX_QUEUED_CHUNKS) chan->tail = 0;
	
	if (chan->tail==chan->head)
	{
		staterr("SS: overqueued channel %d; Bad Things about to happen", c);
		SSUnlockAudio();
		return -1;
	}
	SSUnlockAudio();
	
	//stat("SSEnqueued buffer %d: %08x of %d bytes to channel %d (containing %d samples); UD %d", loc, buffer, chunk->bytelength, c, len, chunk->userdata);
	return c;
}

// works like SSEnqueueChunk, only it does not enqueue. Instead, if a sound
// is already playing on the channel, it is stopped and the new sound takes it's place.
// if c==-1, it acts identically to SSEnqueueChunk since a "free channel" by definition
// has no existing sound to be affected by a queueing operation.
int SSPlayChunk(int c, signed short *buffer, int len, int userdata, void(*FinishedCB)(int, int))
{
	if (c != -1) SSAbortChannel(c);
	
	return SSEnqueueChunk(c, buffer, len, userdata, FinishedCB);
}

// returns true if channel c is currently playing
char SSChannelPlaying(int c)
{
int result;

	SSLockAudio();
	result = (channel[c].head != channel[c].tail);
	SSUnlockAudio();
	
	return result;
}

// returns the userdata member of the currently playing chunk on channel c.
// if channel c is not playing, the results are undefined.
int SSGetCurUserData(int c)
{
int result;

	SSLockAudio();
	
	if (channel[c].head != channel[c].tail)
	{
		result = (channel[c].chunks[channel[c].head].userdata);
	}
	else
	{
		staterr("SSGetUserData: channel %d is not playing!\n", c);
		result = -1;
	}
	
	SSUnlockAudio();
	return result;
}

// returns the currently playing sample within the currently playing chunk
// of channel c. If no chunk is playing, the results are undefined.
// as with "len" parameter to SSEnqueueChunk, the count counts the two
// components of a stereo sample as a single sample.
int SSGetSamplePos(int c)
{
int result;

	SSLockAudio();
	
	if (channel[c].head != channel[c].tail)
	{
		result = (channel[c].chunks[channel[c].head].bytepos / 4);
	}
	else
	{
		staterr("SSGetSamplePos: channel %d is not playing!", c);
		result = -1;
	}
	
	SSUnlockAudio();
	return result;
}


// if a sound is playing on channel c, stops it immediately.
// if not, does nothing.
void SSAbortChannel(int c)
{
	SSLockAudio();
	
	/*if (c >= 0 && c < SS_NUM_CHANNELS)
	{
		while(channel[c].head != channel[c].tail)
		{
			if (channel[c].FinishedCB)
				(*channel[c].FinishedCB)(c, channel[c].chunks[channel[c].head].userdata);
			
			if (++channel[c].head >= MAX_QUEUED_CHUNKS)
				channel[c].head = 0;
		}
	}*/
	channel[c].head = channel[c].tail;
	
	SSUnlockAudio();
}


// aborts all sounds with a userdata value matching "ud".
void SSAbortChannelByUserData(int ud)
{
int c;
	SSLockAudio();
	for(c=0;c<SS_NUM_CHANNELS;c++)
	{
		if (SSChannelPlaying(c) && SSGetCurUserData(c)==ud)
			SSAbortChannel(c);
	}
	SSUnlockAudio();
}

// changes the volume of a channel.
// any currently playing chunks are immediately affected, and any future chunks queued
// will have the new volume setting, until the SSSetVolume function is removed.
void SSSetVolume(int c, int newvol)
{
	SSLockAudio();
	channel[c].volume = newvol;
	SSUnlockAudio();
}

/*
void c------------------------------() {}
*/

// the effects of SSLockAudio are cumulative--calling it more than once will lock
// the audio "more", and you have to call it the same numbers of times before it will unlock.
void SSLockAudio(void)
{
	if (lockcount==0) SDL_LockAudio();
	lockcount++;
}

void SSUnlockAudio(void)
{
	lockcount--;
	if (!lockcount) SDL_UnlockAudio();
}

/*
void c------------------------------() {}
*/

