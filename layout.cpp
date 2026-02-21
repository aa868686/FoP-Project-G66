#include "layout.h"

namespace ui {

    layout build_layout ( int w , int h , const layout_config & cfg ) {
        layout la {} ;

        const int topH = static_cast <int> ( h * cfg.top_bar_height_ratio) ;
        const int leftW = static_cast <int> ( w * cfg.left_panel_width_ratio ) ;
        const int stageW = static_cast <int> ( w * cfg.stage_width_ratio ) ;
        const int spriteH = static_cast <int> ( h * cfg.sprite_bar_height_ratio ) ;

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


    SDL_Rect topbar_menu_rect ( const SDL_Rect & top_bar , int index , int w , int h ) {
        const int pad = 4 ;
        const int start_x = top_bar.x + pad ;
        const int y = top_bar.y + ( ( top_bar.h - h ) / 2 ) ;
        const int x = start_x + ( index * ( w + pad ) ) ;
        return SDL_Rect { x , y , w , h } ;
    }


    SDL_Rect topbar_right_rect ( const SDL_Rect & top_bar , int index , int w , int h ) {
        const int pad = 6 ;
        const int y = top_bar.y + ( ( top_bar.h - h ) / 2 ) ;
        const int right_x = top_bar.x + top_bar.w - pad - w ;
        const int x = right_x - ( index * ( w + pad ) ) ;
        return SDL_Rect { x , y , w , h } ;
    }


    static void fill_rect ( SDL_Renderer * ren , const SDL_Rect &r ) {
        SDL_RenderFillRect ( ren , &r ) ;
    }

    static void draw_rect_outline ( SDL_Renderer *ren , const SDL_Rect &r ) {
        SDL_RenderDrawRect ( ren , &r ) ;
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


        // Background fills
        SDL_SetRenderDrawColor ( ren , 30 , 30 , 30 , 255 ) ;
        fill_rect ( ren , la.topBar ) ;

        SDL_SetRenderDrawColor ( ren , 24 , 24 , 24 , 255 ) ;
        fill_rect ( ren , la.leftPanel ) ;

        SDL_SetRenderDrawColor ( ren , 20 , 20 , 20 , 255 ) ;
        fill_rect ( ren , la.workspace ) ;

        SDL_SetRenderDrawColor ( ren , 26 , 26 , 26 , 255 ) ;
        fill_rect ( ren , la.spriteBar ) ;

        SDL_SetRenderDrawColor ( ren , 18 , 18 , 18 , 255 ) ;
        fill_rect ( ren , la.stage ) ;


        // Panel borders
        SDL_SetRenderDrawColor ( ren , 60 , 60 , 60 , 255 ) ;
        draw_rect_outline ( ren , la.topBar ) ;
        draw_rect_outline ( ren , la.leftPanel ) ;
        draw_rect_outline ( ren , la.workspace ) ;
        draw_rect_outline ( ren , la.stage ) ;
        draw_rect_outline ( ren , la.spriteBar ) ;

    }

}