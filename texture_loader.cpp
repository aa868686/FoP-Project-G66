#include "texture_loader.h"
#include <SDL2/SDL_image.h>
#include <cstdio>

namespace gfx {

    SDL_Texture * load_texture ( SDL_Renderer * renderer ,
                                 const std :: string & file_path ,
                                 int & out_w ,
                                 int & out_h
                                 ) {
        out_w = 0 ;
        out_h = 0 ;

        if ( !renderer ) {
            std :: fprintf ( stderr , "load_texture: renderer is null\n" ) ;
            return nullptr ;
        }

        SDL_Texture * tex = IMG_LoadTexture ( renderer , file_path . c_str() ) ;
        if ( !tex ) {
            std :: fprintf ( stderr ,
                             "IMG_LoadTexture failed for '%s' : %s\n" ,
                             file_path . c_str() ,
                             IMG_GetError()
                             ) ;
            return nullptr ;
        }

        // Query texture size
        int w = 0 , h = 0 ;
        if (SDL_QueryTexture ( tex , nullptr , nullptr , &w , &h ) != 0 ) {
            std :: fprintf ( stderr ,
                             "SDL_QueryTexture failed for '%s': %s\n" ,
                             file_path . c_str () , SDL_GetError()
                             ) ;
            SDL_DestroyTexture ( tex ) ;
            return nullptr ;
        }

        out_w = w ;
        out_h = h ;
        return tex ;
    }
}
