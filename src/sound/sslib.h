
#ifndef _SSLIB_H
#define _SSLIB_H

#define SAMPLE_RATE			22050
#define MAX_QUEUED_CHUNKS		(180 +1)
#define SS_NUM_CHANNELS			16

struct SSChunk
{
	signed short *buffer;
	int length;
	
	signed char *bytebuffer;			// same as bytebuffer but in BYTES
	int bytelength;						// TOTAL length in BYTES
	
	// current read position. this is within bytebuffer and is in BYTES.
	int bytepos;
	
	int userdata;						// user data to be sent to FinishedCallback when finished
};


struct SSChannel
{
	SSChunk chunks[MAX_QUEUED_CHUNKS];
	int head, tail;
	
	int volume;
	char reserved;						// if 1, can only be played on explicitly, not by passing -1
	
	int FinishedChunkUserdata[MAX_QUEUED_CHUNKS];
	int nFinishedChunks;
	
	void (*FinishedCB)(int channel, int chunkid);
};

char SSInit(void);
void SSClose(void);
void SSReserveChannel(int c);
int SSFindFreeChannel(void);
int SSEnqueueChunk(int c, signed short *buffer, int len, int userdata, void(*FinishedCB)(int, int));
int SSPlayChunk(int c, signed short *buffer, int len, int userdata, void(*FinishedCB)(int, int));
char SSChannelPlaying(int c);
int SSGetCurUserData(int c);
int SSGetSamplePos(int c);
void SSAbortChannel(int c);
void SSAbortChannelByUserData(int ud);
void SSSetVolume(int c, int newvol);
void SSLockAudio(void);
void SSUnlockAudio(void);

#endif
