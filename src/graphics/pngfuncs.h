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

//#include <png.h>
#include <SDL.h>

//Not used there but causes libpng compiling error
//static int png_colortype_from_surface(SDL_Surface *surface);
//void png_user_warn(png_structp ctx, png_const_charp str);
//void png_user_error(png_structp ctx, png_const_charp str);
int png_save_surface(char *filename, SDL_Surface *surf);
