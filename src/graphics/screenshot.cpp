#include "pngfuncs.h"
#include "graphics.h"
#include "../common/stat.h"
#if defined(__MACH__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>


#include <sys/stat.h>
namespace Screenshot
{
        /*Generate screenshot name. From MPlayer */
        static int Exists( char *name )
        {
            struct stat dummy;
            if( stat( name, &dummy ) == 0 )
                return 1;
            return 0;
        }

        static void GenerateName( char* name )
        {
            int iter = 0;
            do{
            snprintf( name, 50, "screenshot%03d.png", ++iter);
            } while( Exists( name ) && iter < 1000 );
            if( Exists( name ) )
            {
                name[0] = '\0';
            }
        }
}


void SaveScreenshot()
{
        char Filename[52];
        SDL_Rect viewport;
        SDL_Surface* surface;

        Screenshot::GenerateName( Filename );
        if( Filename[0] == '\0' ){
                stat("Can not get screenshot name. Too many screenshots in folder");
                return;
        }
        SDL_RenderGetViewport(renderer, &viewport);
        surface = SDL_CreateRGBSurface(0, viewport.w, viewport.h, 24,
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
        #else
            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
        #endif
        );

        if (!surface)
        {
            stat("Couldn't create surface: %s\n", SDL_GetError());
            return;
        }

        if ( SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) < 0 )
        {
            stat("Couldn't read screen: %s\n", SDL_GetError());
            return;
        }

        if( png_save_surface(Filename, surface) < 0 ){
                SDL_FreeSurface( surface );
                return;
        }
        SDL_FreeSurface( surface );
        return;

}



