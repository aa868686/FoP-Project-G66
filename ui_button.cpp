#include "ui_button.h"
#include "font_manager.h"

namespace ui {

    bool button_hit ( const button &b , int x , int y ) {
        return ( x >= b.rect.x ) && ( x < b.rect.x + b.rect.w ) &&
               ( y >= b.rect.y ) && ( y < b.rect.y + b.rect.h ) ;
    }


    bool button_handle_click ( const button &b , int x , int y ) {
        if ( !b.enabled ) {
            return false ;
        }
        if ( !button_hit ( b , x , y ) ) {
            return false ;
        }

        if ( b.on_click ) {
            b.on_click() ;
        }

        return true ;
    }


    void button_draw ( SDL_Renderer *r , const button &b , TTF_Font * font , const char * label , SDL_Color fill , SDL_Color border ) {
        if ( !r ) {
            return;
        }

        //Fill
        SDL_SetRenderDrawColor ( r , fill.r , fill.g , fill.b , fill.a ) ;
        SDL_RenderFillRect ( r , &b.rect ) ;

        // Border
        SDL_SetRenderDrawColor ( r , border.r , border.g , border.b , border.a ) ;
        SDL_RenderDrawRect ( r  ,&b.rect ) ;

        if ( font && label ) {
            fnt :: draw_text_centered ( r , font , label , b.rect , { 220 , 220 , 220 , 255 } ) ;
        }


    }

}