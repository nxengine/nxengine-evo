#ifndef _PALETTE_H
#define _PALETTE_H
#include <SDL.h>

void palette_reset(void);
SDL_Surface *palette_add(SDL_Surface *sfc);
int palette_alloc(uint8_t r, uint8_t g, uint8_t b);



#endif
