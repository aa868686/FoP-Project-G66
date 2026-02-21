#include "font_manager.h"
#include <cstdio>

namespace fnt {

    bool font_init ( font_manager & fm , const std :: string & ttf_path ) {
        if ( TTF_Init() != 0 ) {
            std :: fprintf ( stderr , "TTF_Init failed: %s\n" , TTF_GetError() ) ;
            return false ;
        }

        fm.small = TTF_OpenFont ( ttf_path.c_str() , 12 ) ;
        fm.medium = TTF_OpenFont ( ttf_path.c_str() , 14 ) ;
        fm.large = TTF_OpenFont ( ttf_path.c_str() , 16 ) ;

        if ( !fm.small || !fm.medium || !fm.large ) {
            std :: fprintf ( stderr , "TTF_OpenFont failed for '%s': %s\n" ,
                           ttf_path.c_str() , TTF_GetError() ) ;
            font_quit ( fm ) ;
            return false ;
        }
        return true ;
    }


    void font_quit ( font_manager & fm ) {
        if ( fm.small ) { TTF_CloseFont ( fm.small  ) ; fm.small  = nullptr ; }
        if ( fm.medium ) { TTF_CloseFont ( fm.medium ) ; fm.medium = nullptr ; }
        if ( fm.large ) { TTF_CloseFont ( fm.large  ) ; fm.large  = nullptr ; }
        TTF_Quit() ;
    }


    void draw_text_centered ( SDL_Renderer * ren ,
                              TTF_Font * font ,
                              const char * text ,
                              SDL_Rect rect ,
                              SDL_Color color ) {
        if ( !ren || !font || !text ) {
            return ;
        }

        SDL_Surface * surf = TTF_RenderText_Blended ( font , text , color ) ;
        if ( !surf ) {
            return ;
        }

        SDL_Texture * tex = SDL_CreateTextureFromSurface ( ren , surf ) ;
        SDL_FreeSurface ( surf ) ;
        if ( !tex ) {
            return ;
        }

        int tw = 0 , th = 0 ;
        SDL_QueryTexture ( tex , nullptr , nullptr , &tw , &th ) ;

        SDL_Rect dst {
                rect.x + ( rect.w - tw ) / 2 ,
                rect.y + ( rect.h - th ) / 2 ,
                tw , th
        } ;

        SDL_RenderCopy ( ren , tex , nullptr , &dst ) ;
        SDL_DestroyTexture ( tex ) ;
    }


    void draw_text_left ( SDL_Renderer * ren ,
                          TTF_Font * font ,
                          const char * text ,
                          SDL_Rect rect ,
                          SDL_Color color ,
                          int pad_x ) {

        if ( !ren || !font || !text ) {
            return ;
        }

        SDL_Surface * surf = TTF_RenderText_Blended ( font , text , color ) ;
        if ( !surf ) {
            return ;
        }

        SDL_Texture * tex = SDL_CreateTextureFromSurface ( ren , surf ) ;
        SDL_FreeSurface ( surf ) ;
        if ( !tex ) {
            return ;
        }

        int tw = 0 , th = 0 ;
        SDL_QueryTexture ( tex , nullptr , nullptr , &tw , &th ) ;

        const int max_w = rect.w - pad_x * 2 ;
        SDL_Rect src { 0 , 0 , std :: min ( tw , max_w ) , th } ;

        SDL_Rect dst {
                rect.x + pad_x ,
                rect.y + ( rect.h - th ) / 2 ,
                src.w , th
        } ;

        SDL_RenderCopy ( ren , tex , &src , &dst ) ;
        SDL_DestroyTexture ( tex ) ;
    }

}
