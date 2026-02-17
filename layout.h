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


    // Builds layout rectangles based on window size.
    layout build_layout ( int window_w , int window_h ) ;

    // Returns true if point (x,y) lies inside rectangle r.
    bool point_in_rect ( int x , int y , const SDL_Rect &r ) ;

    // Renders layout panels (background + borders).
    void render_layout ( SDL_Renderer *renderer , const layout & layout ) ;

}

#endif //FOP_PROJECT_G66_LAYOUT_H
