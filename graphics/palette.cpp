
#include "../nx.h"
#include "palette.h"
#include "graphics.h"

#define MAX_COLORS		256
static SDL_Color screenpal[MAX_COLORS];
int ncolors = -1;

// clear out all palette entries
void palette_reset(void)
{
	ncolors = 0;
}

// given a paletted surface add it's colors in to the screen colormap
// then return a surface with the color indexes remapped ready to
// be displayed on the screen. insfc is either freed, or reused to
// create the returned surface.
SDL_Surface *palette_add(SDL_Surface *sfc)
{
SDL_Palette *pal = sfc->format->palette;
int remap[MAX_COLORS];
int x, y, i;

	if (sfc->format->BitsPerPixel > 8)
	{
		staterr("palette_add: input surface is > 8bpp");
		return NULL;
	}
	
	stat("palette_add: adding %d colors to screen palette...", pal->ncolors);
	for(i=0;i<pal->ncolors;i++)
	{
		remap[i] = palette_alloc(pal->colors[i].r, pal->colors[i].g, pal->colors[i].b);
		if (remap[i] == -1)
			return sfc;
	}
	
	SDL_SetPaletteColors(screen->Format()->palette, screenpal, 0, ncolors);
	return sfc;
/*
	// remap indexes in surface
	for(y=0;y<sfc->h;y++)
	{
		uint8_t *pixels = (uint8_t *)sfc->pixels + (y * sfc->pitch);
		
		for(x=0;x<sfc->w;x++)
		{
			*pixels = remap[*pixels];
			pixels++;
		}
	}
	
	return sfc;*/
}


// add the given color to the screen palette and return it's index.
int palette_alloc(uint8_t r, uint8_t g, uint8_t b)
{
int i;

	for(i=0;i<ncolors;i++)
	{
		if (screenpal[i].r == r && \
			screenpal[i].g == g && \
			screenpal[i].b == b)
		{
			return i;
		}
	}
	
	if (ncolors >= MAX_COLORS)
	{
		staterr("palette_alloc: out of color space!");
		return -1;
	}
	
	screenpal[ncolors].r = r;
	screenpal[ncolors].g = g;
	screenpal[ncolors].b = b;
	return ncolors++;
}






