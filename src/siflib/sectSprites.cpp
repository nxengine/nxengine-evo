#include "../common/bufio.h"
#include "../common/stat.h"
#include <cstring>

#include "sectSprites.h"


int SIFSpritesSect::GetSpriteCount(const uint8_t *data, int datalen)
{
	const uint8_t *data_end = data + (datalen - 1);
	return read_U16(&data, data_end);
}


bool SIFSpritesSect::Decode(const uint8_t *data, int datalen, \
						  SIFSprite *sprites, int *nsprites_out, int maxsprites)
{
const uint8_t *data_end = data + (datalen - 1);
int i, f, nsprites;
	
	nsprites = read_U16(&data, data_end);
	if (nsprites_out) *nsprites_out = nsprites;
	
	if (nsprites >= maxsprites)
	{
		staterr("SIFSpritesSect::Decode: too many sprites in file (nsprites=%d, maxsprites=%d)", nsprites, maxsprites);
		return 1;
	}
	
	stat("SIFSpritesSect: loading %d sprites", nsprites);
	for(i=0;i<nsprites;i++)
	{
		if (data > data_end)
		{
			staterr("SIFSpritesSect::Decode: section corrupt: overran end of data");
			return 1;
		}
		
		// read sprite-level fields
		sprites[i].w = read_U8(&data, data_end);
		sprites[i].h = read_U8(&data, data_end);
		sprites[i].spritesheet = read_U8(&data, data_end);
		
		sprites[i].nframes = read_U8(&data, data_end);
		sprites[i].ndirs = read_U8(&data, data_end);
		
		if (sprites[i].ndirs > SIF_MAX_DIRS)
		{
			staterr("SIFSpritesSect::Decode: SIF_MAX_DIRS exceeded on sprite %d (ndirs=%d)", i, sprites[i].ndirs);
			return 1;
		}
		
		LoadRect(&sprites[i].bbox, &data, data_end);
		LoadRect(&sprites[i].solidbox, &data, data_end);
		
		LoadPoint(&sprites[i].spawn_point, &data, data_end);
		
		LoadPointList(&sprites[i].block_l, &data, data_end);
		LoadPointList(&sprites[i].block_r, &data, data_end);
		LoadPointList(&sprites[i].block_u, &data, data_end);
		LoadPointList(&sprites[i].block_d, &data, data_end);
		
		// malloc enough space to hold the specified number
		// of apple fritters, i mean, frames.
		sprites[i].frame = (SIFFrame *)malloc(sizeof(SIFFrame) * sprites[i].nframes);
		
		// then load all frames
		for(f=0;f<sprites[i].nframes;f++)
		{
			if (LoadFrame(&sprites[i].frame[f], sprites[i].ndirs, &data, data_end))
				return 1;
		}
	}
	
	return 0;
}

bool SIFSpritesSect::LoadFrame(SIFFrame *frame, int ndirs, \
					const uint8_t **data, const uint8_t *data_end)
{
	// sets defaults for un-specified/default fields
	memset(frame, 0, sizeof(SIFFrame));
	
	for(int d=0;d<ndirs;d++)
	{
		SIFDir *dir = &frame->dir[d];
		LoadPoint(&dir->sheet_offset, data, data_end);
		
		int t;
		for(;;)
		{
			t = read_U8(data, data_end);
			if (t == S_DIR_END) break;
			
			switch(t)
			{
				case S_DIR_DRAW_POINT: LoadPoint(&dir->drawpoint, data, data_end); break;
				case S_DIR_ACTION_POINT: LoadPoint(&dir->actionpoint, data, data_end); break;
				case S_DIR_ACTION_POINT_2: LoadPoint(&dir->actionpoint2, data, data_end); break;
				
				case S_DIR_PF_BBOX:
					LoadRect(&dir->pf_bbox, data, data_end);
				break;
				
				default:
					stat("SIFSpriteSect::LoadFrame: encountered unknown optional field type %d", t);
				return 1;
			}
		}
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

void SIFSpritesSect::LoadRect(SIFRect *rect, const uint8_t **data, const uint8_t *data_end)
{
	rect->x1 = (int16_t)read_U16(data, data_end);
	rect->y1 = (int16_t)read_U16(data, data_end);
	rect->x2 = (int16_t)read_U16(data, data_end);
	rect->y2 = (int16_t)read_U16(data, data_end);
}

void SIFSpritesSect::LoadPoint(SIFPoint *pt, const uint8_t **data, const uint8_t *data_end)
{
	pt->x = (int16_t)read_U16(data, data_end);
	pt->y = (int16_t)read_U16(data, data_end);
}

void SIFSpritesSect::LoadPointList(SIFPointList *lst, const uint8_t **data, const uint8_t *data_end)
{
	lst->count = read_U8(data, data_end);
	if (lst->count > SIF_MAX_BLOCK_POINTS)
	{
		staterr("SIFSpritesSect::LoadPointList: too many block points (%d, max=%d)", lst->count, SIF_MAX_BLOCK_POINTS);
		return;
	}
	
	for(int i=0;i<lst->count;i++)
	{
		lst->point[i].x = (int16_t)read_U16(data, data_end);
		lst->point[i].y = (int16_t)read_U16(data, data_end);
	}
}

