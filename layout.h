#ifndef FOP_PROJECT_G66_LAYOUT_H
#define FOP_PROJECT_G66_LAYOUT_H

#pragma once

#include <SDL2/SDL.h>

namespace ui {

    struct layout {
        SDL_Rect topBar ;
        SDL_Rect leftPanel ;
        SDL_Rect workspace ;
        SDL_Rect stage ;
        SDL_Rect spriteBar ;
        SDL_Rect spriteInfo ;
    };

      struct layout_config {
          float top_bar_height_ratio = 0.065f ;
          float left_panel_width_ratio = 0.25f ;
          float stage_width_ratio = 0.38f ;
          float sprite_bar_height_ratio = 0.20f ;
      };


    layout build_layout ( int window_w , int window_h ,
                          const layout_config & cfg = layout_config {}
                          ) ;

    bool point_in_rect ( int x , int y , const SDL_Rect &r ) ;

    void render_layout ( SDL_Renderer *renderer , const layout & la ) ;


    SDL_Rect topbar_menu_rect ( const SDL_Rect & top_bar ,
                                int index ,
                                int w = 72 ,
                                int h = 26
                                ) ;


    SDL_Rect topbar_right_rect ( const SDL_Rect & top_bar ,
                                 int index ,
                                 int w = 28 ,
                                 int h = 28
                                 ) ;
}

#endif //FOP_PROJECT_G66_LAYOUT_H
