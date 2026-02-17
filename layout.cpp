#include "layout.h"

namespace ui {

    layout build_layout ( int w , int h ) {
        const int topH = 40 ;
        const int leftW = 280 ;
        const int stageW = 420 ;
        const int spriteH = 140 ;

        layout la{} ;

        la.topBar = { 0 , 0 , w , topH } ;
        la.leftPanel = { 0 , topH , leftW , h - topH } ;
        la.stage = { w - stageW , topH , stageW , h - topH - spriteH } ;
        la.spriteBar = { w - stageW , h - spriteH , stageW , spriteH } ;
        la.workspace = { leftW , topH , w - leftW - stageW , h - topH } ;

        return la ;
    }


    bool point_in_rect ( int x , int y , const SDL_Rect &r ) {
        return ( x >= r.x && x < r.x + r.w &&
                 y >= r.y && y < r.y + r.h ) ;
    }

    static void draw_rect ( SDL_Renderer *ren , const SDL_Rect &r , bool filled ) {
        if ( filled ) {
            SDL_RenderFillRect ( ren , &r ) ;
        } else {
            SDL_RenderDrawRect ( ren , &r ) ;
        }
    }

    void render_layout ( SDL_Renderer * ren , const layout & la ) {
        if ( !ren ) {
            return;
        }


        // Fill panels
        SDL_SetRenderDrawColor ( ren , 30 , 30 , 30 , 255 ) ;
        draw_rect ( ren , la.topBar , true ) ;

        SDL_SetRenderDrawColor ( ren , 24 , 24 , 24 , 255 ) ;
        draw_rect ( ren , la.leftPanel , true ) ;

        SDL_SetRenderDrawColor ( ren , 20 , 20 , 20 , 255 ) ;
        draw_rect ( ren , la.workspace , true ) ;

        SDL_SetRenderDrawColor ( ren , 26 , 26 , 26 , 255 ) ;
        draw_rect ( ren , la.spriteBar , true ) ;


        // Borders
        SDL_SetRenderDrawColor ( ren , 70 , 70 , 70 , 255 ) ;
        draw_rect ( ren , la.topBar , false ) ;
        draw_rect ( ren , la.leftPanel , false ) ;
        draw_rect ( ren , la.workspace , false ) ;
        draw_rect ( ren , la.stage , false ) ;
        draw_rect ( ren , la.spriteBar , false ) ;

    }


}