#include "ui_menu.h"
#include "font_manager.h"

namespace ui {

    static bool pt_in_rect ( const SDL_Rect & rc , int x , int y ) {
        return ( x >= rc.x ) && ( x < rc.x + rc.w ) &&
               ( y >= rc.y ) && ( y < rc.y + rc.h ) ;
    }


    void menu_layout ( menu &m, TTF_Font* font ) {
        const int item_count = static_cast < int > ( m.items.size() ) ;
        if ( item_count <= 0 ) {
            m.panel_rect = SDL_Rect { m.title_rect.x , m.title_rect.y + m.title_rect.h , m.title_rect.w , 0 } ;
            return ;
        }
        int max_w = m.title_rect.w;

        if (font) {
            for (const auto& it : m.items) {
                int text_w = 0, text_h = 0;
                TTF_SizeText(font, it.label, &text_w, &text_h);
                int needed = text_w + 24; // 24px padding
                if (needed > max_w) max_w = needed;
            }
        }



        m.panel_rect.x = m.title_rect.x ;
        m.panel_rect.y = m.title_rect.y + m.title_rect.h ;
        m.panel_rect.w = max_w;
        m.panel_rect.h = item_count * m.item_height ;

        for ( int i = 0 ; i < item_count ; ++i ) {
            m.items[i].rect.x = m.panel_rect.x ;
            m.items[i].rect.y = m.panel_rect.y + ( i * m.item_height ) ;
            m.items[i].rect.w = m.panel_rect.w ;
            m.items[i].rect.h = m.item_height ;
        }
    }


    void menu_draw ( SDL_Renderer *r , const menu &m ,
                     TTF_Font * font ,
                     SDL_Color title_fill , SDL_Color title_border ,
                     SDL_Color panel_fill , SDL_Color panel_border ,
                     SDL_Color item_fill , SDL_Color item_border ,
                     SDL_Color item_disabled_fill
                     ) {
        if ( !r ) {
            return ;
        }

        SDL_Color t_fill = m.title_hovered ? SDL_Color { 70 , 70 , 70 , 255 } : title_fill ;
        SDL_SetRenderDrawColor ( r , t_fill.r , t_fill.g , t_fill.b , t_fill.a ) ;
        SDL_RenderFillRect ( r , &m.title_rect ) ;
        SDL_SetRenderDrawColor ( r , title_border.r , title_border.g , title_border.b , title_border.a ) ;
        SDL_RenderDrawRect ( r , &m.title_rect ) ;


        fnt :: draw_text_centered ( r , font , m.title , m.title_rect , { 220 , 220 , 220 , 255 } ) ;

        /////Title text will be added/////

        if ( !m.open ) {
            return ;
        }


        SDL_SetRenderDrawColor ( r , panel_fill.r , panel_fill.g , panel_fill.b , panel_fill.a ) ;
        SDL_RenderFillRect ( r , &m.panel_rect ) ;
        SDL_SetRenderDrawColor ( r , panel_border.r , panel_border.g , panel_border.b , panel_border.a ) ;
        SDL_RenderDrawRect ( r , &m.panel_rect ) ;


        for ( const auto & it : m.items ) {
            SDL_Color fill = !it.enabled ? item_disabled_fill : it.hovered ? SDL_Color { 80 , 80 , 80 , 255 } : item_fill ;

            SDL_SetRenderDrawColor ( r , fill.r , fill.g , fill.b , fill.a ) ;
            SDL_RenderFillRect ( r , &it.rect ) ;


            SDL_SetRenderDrawColor ( r , item_border.r , item_border.g , item_border.b , item_border.a ) ;
            SDL_RenderDrawRect ( r , &it.rect ) ;

            fnt :: draw_text_left ( r , font , it.label , it.rect , { 200 , 200 , 200 , 255 } ) ;

        }
    }


    bool menu_handle_click ( menu &m , int x , int y ) {
        // Always recompute layout before hit-testing
        menu_layout ( m , nullptr) ;

        // Click on title toggles menu
        if ( pt_in_rect ( m.title_rect , x , y ) ) {
            m.open = !m.open ;
            return true ;
        }

        if ( !m.open ) {
            return false ;
        }


        // Click inside panel: choose item
        if ( pt_in_rect ( m.panel_rect , x , y ) ) {
            for ( auto &it : m.items ) {
                if ( !pt_in_rect ( it.rect , x , y ) ) {
                    continue ;
                }

                if ( it.enabled && it.action ) {
                    it.action() ;
                }
                m.open = false ; // Close after selection
                return true ;
            }
            // Clicked panel but not on an item
            return true ;
        }
        // Click outside -> not consumed here
        return false ;
    }

    void menu_close ( menu &m ) {
        m.open = false ;
    }

}