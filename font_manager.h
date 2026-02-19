#ifndef FOP_PROJECT_G66_FONT_MANAGER_H
#define FOP_PROJECT_G66_FONT_MANAGER_H

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

namespace fnt {

    struct font_manager {
        TTF_Font * small = nullptr ;
        TTF_Font * medium = nullptr ;
        TTF_Font * large = nullptr ;
    };

    bool font_init ( font_manager & fm , const std :: string & ttf_path ) ;

    void font_quit ( font_manager & fm ) ;

    void draw_text_centered ( SDL_Renderer *ren ,
                              TTF_Font * font ,
                              const char * text ,
                              SDL_Rect rect ,
                              SDL_Color color
                              ) ;

    void draw_text_left ( SDL_Renderer * ren , TTF_Font * font ,
                          const char * text ,
                          SDL_Rect rect , SDL_Color color ,
                          int pad_x = 6
                          ) ;


}

#endif //FOP_PROJECT_G66_FONT_MANAGER_H
