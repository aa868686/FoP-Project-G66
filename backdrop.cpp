#include "backdrop.h"
#include <algorithm>

namespace gfx {

    backdrop_manager backdrop_make_defaults () {
        backdrop_manager bm {} ;

        backdrop b1 {} ;
        b1.name = "Blue Sky" ;
        b1.color = SDL_Color { 70 , 130 , 180 , 255 } ;
        bm.backdrops.push_back ( b1 ) ;

        backdrop b2 {} ;
        b2.name = "Green Field" ;
        b2.color = SDL_Color { 60 , 140 , 70 , 255 } ;
        bm.backdrops.push_back ( b2 ) ;

        bm.active = 0 ;

        return bm ;
    }


    int backdrop_add_color ( backdrop_manager & bm ,
                             const std :: string &name ,
                             SDL_Color color
                             ) {
        backdrop b {} ;
        b.name = name ;
        b.color = color ;
        bm.backdrops.push_back ( b ) ;
        return static_cast <int> ( bm.backdrops.size() ) - 1 ;
    }

    int backdrop_add_texture ( backdrop_manager & bm ,
                               const std :: string & name ,
                               SDL_Texture *tex ,
                               int tex_w , int tex_h ) {
        backdrop b {} ;
        b.name = name ;
        b.texture = tex ;
        b.tex_w = tex_w ;
        b.tex_h = tex_h ;
        bm.backdrops.push_back ( b ) ;
        return static_cast <int> ( bm.backdrops.size() ) - 1 ;
    }


    bool backdrop_set_active ( backdrop_manager & bm  , int index ) {
        if ( index < 0 || index >= static_cast <int> ( bm.backdrops.size() ) ) {
            return false ;
        }
        bm.active = index ;
        return true ;
    }


    bool backdrop_set_active_by_name ( backdrop_manager & bm ,
                                       const std :: string & name ) {
        for ( int i = 0 ; i < static_cast <int> ( bm.backdrops.size() ) ; ++i ) {
            if ( bm.backdrops[i].name == name ) {
                bm.active = i ;
                return true ;
            }
        }
        return false ;
    }


    void backdrop_render ( SDL_Renderer * ren ,
                           const backdrop_manager & bm ,
                           SDL_Rect stage ) {
        if ( !ren || bm.backdrops.empty() ) {
            return ;
        }

        const int idx = std :: max ( 0 , std :: min ( bm.active , static_cast <int> ( bm.backdrops.size() ) -1 ) ) ;

        const backdrop & bd = bm.backdrops[idx] ;

        if ( bd.texture ) {
            SDL_RenderCopy ( ren , bd.texture , nullptr , &stage ) ;
        } else {
            SDL_SetRenderDrawColor ( ren , bd.color.r , bd.color.g , bd.color.b , bd.color.a ) ;
            SDL_RenderFillRect ( ren , &stage ) ;
        }
    }


}
