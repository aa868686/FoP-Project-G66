#ifndef FOP_PROJECT_G66_UI_MENU_H
#define FOP_PROJECT_G66_UI_MENU_H

#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <functional>
#include <vector>

namespace ui {
    struct menuItem {
        // Text label
        const char * label { nullptr } ;

        bool enabled { true } ;
        std :: function <void()> action {} ;

        // Runtime-generated clickable rect (set by menu-layout)
        SDL_Rect rect { 0 , 0 , 0 , 0 } ;
    };

    struct menu {
        const char * title { nullptr } ;

        // Title bar rect (clicking toggles open/close)
        SDL_Rect title_rect { 0 , 0 , 0 , 0 } ;

        // Dropdown panel rect (computed)
        SDL_Rect panel_rect { 0 , 0 , 0 , 0 } ;

        bool open { false } ;
        int item_height { 26 } ;

        std :: vector < menuItem > items {} ;
    };


    // Compute panel_rect and each item.rect based on title_rect and item_height
    void menu_layout ( menu &m ) ;

    // Draw title + dropdown
    void menu_draw ( SDL_Renderer *r , const menu &m ,
                     TTF_Font * font ,
                     SDL_Color title_fill , SDL_Color title_border ,
                     SDL_Color panel_fill , SDL_Color panel_border ,
                     SDL_Color item_fill , SDL_Color item_border ,
                     SDL_Color item_disabled_fill
                     ) ;

    // Handle click; returns true if consumed
    bool menu_handle_click ( menu &m , int x , int y ) ;

    // Utility: close menu (used when clicking elsewhere)
    void menu_close ( menu &m ) ;

}

#endif //FOP_PROJECT_G66_UI_MENU_H
