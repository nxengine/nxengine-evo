
#ifndef _SECT_SPRITES_H
#define _SECT_SPRITES_H

// this decodes and encodes the main sprites[] array of SIFSprite structures.

class DBuffer;
#include "sif.h"


class SIFSpritesSect
{
public:
	// get how many sprites are in the file. You can call this first if you need
	// to know how big to make the array.
	static int GetSpriteCount(const uint8_t *data, int datalen);
	
	// decode from a SIF_SECTION_SPRITES and use the resultant data to fill in
	// an array of SIFSprite structures pointed to by sprites.
	static bool Decode(const uint8_t *data, int datalen, \
					SIFSprite *sprites, int *nsprites_out, int maxsprites);
	
	// read sprites out of the given array of SIFSprite structures and generate
	// a SIF_SECTION_SPRITES record.
	static uint8_t *Encode(SIFSprite *sprites, int nsprites, int *datalen_out);
	
	
private:
	static void LoadRect(SIFRect *rect, const uint8_t **data, const uint8_t *data_end);
	static void LoadPoint(SIFPoint *pt, const uint8_t **data, const uint8_t *data_end);
	static void LoadPointList(SIFPointList *lst, const uint8_t **data, const uint8_t *data_end);
	static bool LoadFrame(SIFFrame *frame, int ndirs, const uint8_t **data, const uint8_t *data_end);
	
	static void SaveRect(SIFRect *rect, DBuffer *out);
	static void SavePoint(SIFPoint *pt, DBuffer *out);
	static void SavePointList(SIFPointList *lst, DBuffer *out);
	static void SaveOptionalPoint(int type, SIFPoint *pt, DBuffer *out);
	static void SaveOptionalRect(int type, SIFRect *rect, DBuffer *out);
	static void SaveFrame(SIFFrame *frame, int ndirs, DBuffer *out);
	
};


// field types within SIFDir which are optional
enum
{
	S_DIR_END,							// ends section list
	
	S_DIR_DRAW_POINT,					// default is [0,0]
	S_DIR_ACTION_POINT,					// default is [0,0]
	S_DIR_ACTION_POINT_2,				// default is [0,0]
	S_DIR_PF_BBOX						// default is no per-frame bbox, or rather [0,0]-[0,0]
};

#endif
