/* SaveSurf: an example on how to save a SDLSurface in PNG
   Copyright (C) 2006 Angelo "Encelo" Theodorou

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   NOTE:

   This program is part of "Mars, Land of No Mercy" SDL examples,
   you can find other examples on http://marsnomercy.org
*/

#include "pngfuncs.h"

#include "../common/misc.h"
#include "../common/stat.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <png.h>

static int png_colortype_from_surface(SDL_Surface *surface)
{
  int colortype = PNG_COLOR_MASK_COLOR; /* grayscale not supported */

  if (surface->format->palette)
    colortype |= PNG_COLOR_MASK_PALETTE;
  else if (surface->format->Amask)
    colortype |= PNG_COLOR_MASK_ALPHA;

  return colortype;
}

void user_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
  fread(data, 1, length, (FILE *)(png_get_progressive_ptr(png_ptr)));
}

void png_user_warn(png_structp ctx, png_const_charp str)
{
  staterr("libpng: warning: %s\n", str);
}

void png_user_error(png_structp ctx, png_const_charp str)
{
  staterr("libpng: error: %s\n", str);
}

int png_save_surface(const std::string& filename, SDL_Surface *surf)
{
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  int i, colortype;
  png_bytep *row_pointers;

  /* Opening output file */
  fp = myfopen(widen(filename).c_str(), widen("wb").c_str());
  if (fp == NULL)
  {
    staterr("fopen error");
    return -1;
  }

  /* Initializing png structures and callbacks */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_user_error, png_user_warn);
  if (png_ptr == NULL)
  {
    staterr("png_create_write_struct error!\n");
    fclose(fp);
    return -1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    staterr("png_create_info_struct error!\n");
    fclose(fp);
    exit(-1);
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    exit(-1);
  }

  png_init_io(png_ptr, fp);

  colortype = png_colortype_from_surface(surf);
  png_set_IHDR(png_ptr, info_ptr, surf->w, surf->h, 8, colortype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  /* Writing the image */
  png_write_info(png_ptr, info_ptr);
  png_set_packing(png_ptr);

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * surf->h);
  for (i = 0; i < surf->h; i++)
    row_pointers[i] = (png_bytep)(Uint8 *)surf->pixels + i * surf->pitch;
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);

  /* Cleaning out... */
  free(row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  return 0;
}

SDL_Surface *png_load_surface(const std::string& name)
{
  png_structp png_ptr;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  int color_type, interlace_type;
  FILE *fp = NULL;

  Uint32 Rmask = 0x00FF0000;
  Uint32 Gmask = 0x0000FF00;
  Uint32 Bmask = 0x000000FF;
  Uint32 Amask = 0xFF000000;

  if ((fp = myfopen(widen(name).c_str(), widen("rb").c_str())) == NULL)
  {
    staterr("Can't open file\n");

    return NULL;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_user_error, png_user_warn);

  if (png_ptr == NULL)
  {
    staterr("libpng: can't create read struct\n");
    fclose(fp);
    return NULL;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL)
  {
    staterr("libpng: can't create info struct\n");
    fclose(fp);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return NULL;
  }
  if (setjmp(png_jmpbuf(png_ptr)))
  {
    staterr("libpng: can't setjmp\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  // png_init_io(png_ptr, fp);
  png_set_read_fn(png_ptr, (png_voidp)fp, user_read_fn);

  png_set_sig_bytes(png_ptr, sig_read);

  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

  png_uint_32 width, height;
  int bit_depth;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

  unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);

  SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, Rmask, Gmask, Bmask, Amask);

  if (surface == NULL)
  {
    printf("5 %s: %d\n", SDL_GetError(), bit_depth);
    return NULL;
  }

  png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

  for (unsigned int i = 0; i < height; i++)
  {
    memcpy(((uint8_t *)surface->pixels) + (row_bytes * i), row_pointers[i], row_bytes);
  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(fp);

  return surface;
}