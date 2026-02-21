#ifndef FOP_PROJECT_G66_UI_MENU_H
#define FOP_PROJECT_G66_UI_MENU_H

#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <functional>
#include <vector>

namespace ui {
    struct menuItem {
        const char * label { nullptr } ;
        bool enabled { true } ;
        std :: function <void()> action {} ;
        SDL_Rect rect { 0 , 0 , 0 , 0 } ;
        bool hovered { false } ;
    };

    struct menu {
        const char * title { nullptr } ;

        SDL_Rect title_rect { 0 , 0 , 0 , 0 } ;
        SDL_Rect panel_rect { 0 , 0 , 0 , 0 } ;

        bool open { false } ;
        bool title_hovered { false } ;
        int item_height { 26 } ;

        std :: vector < menuItem > items {} ;
    };


    void menu_layout ( menu &m, TTF_Font* font) ;

    void menu_draw ( SDL_Renderer *r , const menu &m ,
                     TTF_Font * font ,
                     SDL_Color title_fill , SDL_Color title_border ,
                     SDL_Color panel_fill , SDL_Color panel_border ,
                     SDL_Color item_fill , SDL_Color item_border ,
                     SDL_Color item_disabled_fill
                     ) ;

    bool menu_handle_click ( menu &m , int x , int y ) ;

    void menu_close ( menu &m ) ;

}

#endif //FOP_PROJECT_G66_UI_MENU_H
