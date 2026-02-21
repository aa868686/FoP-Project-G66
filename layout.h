#ifndef FOP_PROJECT_G66_LAYOUT_H
#define FOP_PROJECT_G66_LAYOUT_H

#pragma once

#include <SDL2/SDL.h>

namespace ui {

    //Represents the main IDE layout regions.
    struct layout {
        SDL_Rect topBar ;      // File / Help / Settings / Run
        SDL_Rect leftPanel ;   // Block categories
        SDL_Rect workspace ;   // Block editing area
        SDL_Rect stage ;       // Sprite rendering stage
        SDL_Rect spriteBar ;   // Sprite list / management panel
    };

      struct layout_config {
          float top_bar_height_ratio = 0.065f ;
          float left_panel_width_ratio = 0.25f ; // Block palette
          float stage_width_ratio = 0.38f ; // Preview stage
          float sprite_bar_height_ratio = 0.20f ; // Sprite list at bottom of stage column
      };


    // Builds layout rectangles based on window size.
    layout build_layout ( int window_w , int window_h ,
                          const layout_config & cfg = layout_config {}
                          ) ;

    // Returns true if point (x,y) lies inside rectangle r.
    bool point_in_rect ( int x , int y , const SDL_Rect &r ) ;

    // Renders layout panels (background + borders).
    void render_layout ( SDL_Renderer *renderer , const layout & la ) ;

    // Returns rect for a menu title button in topBar, placed left-to-right.
    // index = 0 -> leftmost, index = 1 -> next, ...
    // w = desired button width, h = desired button height (centered vertically).


    // Returns rect for right-aligned icon/button in topBar.
    // index = 0 -> rightmost, index = 1 -> one slot to the lest, ...
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
