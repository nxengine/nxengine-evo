
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "sif.h"
#include "sifloader.h"
#include "sectSprites.h"
#include "sectStringArray.h"

// safely make some vars sane while still staying POD
void SIFSprite::Init()
{
	frame = NULL;
}

// completely zero ALL vars
void SIFSprite::Zero()
{
	memset(this, 0, sizeof(SIFSprite));
}

void SIFSprite::FreeData()
{
	if (frame)
	{
		free(frame);
		frame = NULL;
	}
}

/*
void c------------------------------() {}
*/

void SIFSprite::CopyFrom(SIFSprite *other)
{
	*this = *other;
	
	// whoops though, we can't share the frame pointer, so create a new copy of that
	int copy_size = (nframes * sizeof(SIFFrame));
	
	frame = (SIFFrame *)malloc(copy_size);
	memcpy(frame, other->frame, copy_size);
}


SIFSprite *SIFSprite::Duplicate()
{
	SIFSprite *spr = new SIFSprite();
	spr->CopyFrom(this);
	
	return spr;
}

/*
void c------------------------------() {}
*/

void SIFSprite::AddFrame(SIFFrame *newframe)
{
	int frameno = nframes;
	SetNumFrames(nframes + 1);
	memcpy(&frame[frameno], newframe, sizeof(SIFFrame));
}

void SIFSprite::InsertFrame(SIFFrame *newframe, int insertbefore)
{
	if (insertbefore < 0) return;
	if (insertbefore >= nframes - 1)
	{
		AddFrame(newframe);
		return;
	}
	
	// copy newframe now--if it's a pointer to one of our own frames, it might get
	// invalidated in a moment when SetNumFrames realloc's.
	SIFFrame insertframe = *newframe;
	
	SetNumFrames(nframes + 1);
	
	int copy_len = ((nframes - 1) - insertbefore) * sizeof(SIFFrame);
	memmove(&frame[insertbefore+1], &frame[insertbefore], copy_len);
	
	frame[insertbefore] = insertframe;
}

void SIFSprite::DeleteFrame(int index)
{
	if (index < 0 || index >= nframes)
		return;
	
	if (index < (nframes - 1))
	{
		int copy_len = ((nframes - 1) - index) * sizeof(SIFFrame);
		memmove(&frame[index], &frame[index+1], copy_len);
	}
	
	SetNumFrames(nframes - 1);
}

void SIFSprite::SetNumFrames(int newcount)
{
	if (newcount == nframes) return;
	
	int required_size = (sizeof(SIFFrame) * newcount);
	if (frame) frame = (SIFFrame *)realloc(frame, required_size);
		  else frame = (SIFFrame *)malloc(required_size);
	
	if (newcount > nframes)
	{
		int blank_size = (newcount - nframes) * sizeof(SIFFrame);
		memset(&frame[nframes], 0, blank_size);
	}
	
	nframes = newcount;
	return;
}

SIFDir *SIFSprite::dir(int f, int d)
{
	if (f < 0 || f >= nframes) return NULL;
	if (d < 0 || d >= ndirs) return NULL;
	if (frame == NULL) return NULL;
	
	return &frame[f].dir[d];
}

/*
void c------------------------------() {}
*/





