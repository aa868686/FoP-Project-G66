#ifndef FOP_PROJECT_G66_BACKDROP_H
#define FOP_PROJECT_G66_BACKDROP_H

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace gfx {

    // A backdrop is either a solid colour or a loaded texture.
    // texture == nullptr means solid colour mode.
    struct backdrop {
        std :: string name {} ;
        SDL_Color color { 70 , 130 , 180 , 255 } ; // fallback solid colour
        SDL_Texture * texture = nullptr ; // optional image
        int tex_w = 0 ;
        int tex_h = 0 ;
    } ;

    // Holds all backdrops available in the project + which one is active.
    struct backdrop_manager {
        std :: vector <backdrop> backdrops {} ;
        int active = 0 ;
    } ;


    backdrop_manager backdrop_make_defaults () ;


    int backdrop_add_color ( backdrop_manager & bm ,
                              const std :: string &name ,
                              SDL_Color color ) ;


    int backdrop_add_texture ( backdrop_manager & bm ,
                                const std :: string & name ,
                                SDL_Texture *tex ,
                                int tex_w , int tex_h ) ;


    bool backdrop_set_active ( backdrop_manager & bm , int index ) ;

    bool backdrop_set_active_by_name ( backdrop_manager & bm ,
                                       const std :: string & name ) ;


    void backdrop_render ( SDL_Renderer *ren ,
                           const backdrop_manager & bm ,
                           SDL_Rect stage ) ;

}

#endif // FOP_PROJECT_G66_BACKDROP_H