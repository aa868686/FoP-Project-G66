#ifndef FOP_PROJECT_G66_UI_BUTTON_H
#define FOP_PROJECT_G66_UI_BUTTON_H

#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <functional>

namespace ui {

    struct button {
        SDL_Rect rect { 0 , 0 , 0 , 0 } ;
        bool enabled { true } ;

        // Called when a click happens inside rect (and enabled == true)
        std :: function <void()> on_click {} ;
    };


    // Basic hit test
    bool button_hit ( const button &b , int x , int y ) ;


    // Triggers on_click if hit (return true if consumed)
    bool button_handle_click ( const button &b , int x , int y ) ;


    // Minimal visual
    void button_draw ( SDL_Renderer *r , const button &b , TTF_Font * font , const char * label , SDL_Color fill , SDL_Color border ) ;

}

#endif //FOP_PROJECT_G66_UI_BUTTON_H
