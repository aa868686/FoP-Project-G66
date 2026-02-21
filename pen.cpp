#include "pen.h"
#include <algorithm>
#include <cmath>

namespace gfx {

    static int clamp_int ( int v , int lo , int hi ) {
        return std :: min ( std :: max ( v, lo ) , hi ) ;
    }

    static void draw_thick_line ( SDL_Renderer * ren ,
                                  float x0 , float y0 ,
                                  float x1 , float y1 ,
                                  SDL_Color color , int thickness
                                  ) {
        if ( !ren ) {
            return ;
        }

        thickness = std :: max ( 1 , thickness ) ;
        SDL_SetRenderDrawBlendMode ( ren , SDL_BLENDMODE_BLEND ) ;
        SDL_SetRenderDrawColor ( ren , color.r , color.g , color.b , color.a ) ;


        if ( thickness == 1 ) {
            SDL_RenderDrawLineF ( ren , x0 , y0 , x1 , y1 ) ;
            return ;
        }

        float dx = x1 - x0 ;
        float dy = y1 - y0 ;
        float len = std :: sqrt ( dx * dx + dy * dy ) ;


        if ( len < 1e-4f ) {
            float half = ( float )( thickness) * 0.5f ;
            SDL_FRect r { x0 - half , y0 - half , ( float ) thickness , ( float ) thickness } ;
            SDL_RenderFillRectF ( ren , &r ) ;
            return ;
        }


        float nx = -dy / len ;
        float ny = dx / len ;

        int r = thickness / 2 ;


        for ( int i = -r ; i <= r ; ++i ) {
            float ox = nx * ( float ) i ;
            float oy = ny * ( float ) i ;
            SDL_RenderDrawLineF ( ren , x0 + ox , y0 + oy , x1 + ox , y1 + oy ) ;

        }
    }



    void pen_init ( pen_state & pen ) {
        pen . is_down = false ;
        pen . color = SDL_Color { 0 , 0 , 0 , 255 } ;
        pen . size = 1 ;

        pen . has_last = false ;
        pen . last_x = 0.0f ;
        pen . last_y = 0.0f ;

        pen . segment . clear () ;
        pen . stamps . clear () ;
        pen . has_last = false ;
    }

    void pen_erase_all ( pen_state & pen ) {
        pen . segment . clear() ;
        pen . stamps . clear() ;
        pen . has_last = false ;
        pen . is_down = false ;
    }


    void pen_down ( pen_state & pen , float x , float y ) {
        pen . is_down = true ;
        pen . has_last = true ;
        pen . last_x = x ;
        pen . last_y = y ;
    }


    void pen_up ( pen_state & pen ) {
        pen . is_down = false ;
        pen . has_last = false ;
    }

    void pen_set_color ( pen_state & pen , SDL_Color c ) {
        pen . color = c ;
    }

    void pen_set_size ( pen_state & pen , int size_px ) {
        pen . size = clamp_int ( size_px , 1 , 200 ) ;
    }

    void pen_change_size ( pen_state & pen , int delta_px ) {
        pen_set_size ( pen , pen.size + delta_px ) ;
    }

    void pen_add_point ( pen_state & pen , float x , float y ) {
        if ( !pen.is_down ) {
            return ;
        }

        if ( !pen.has_last ) {
            pen . has_last = true ;
            pen . last_x = x ;
            pen . last_y = y ;
            return ;
        }


        PenSegment seg ;
        seg . x0 = pen . last_x ;
        seg . y0 = pen . last_y ;
        seg . x1  = x ;
        seg . y1 = y ;
        seg . color = pen . color ;
        seg . size = pen . size ;

        pen . segment . push_back ( seg ) ;

        pen . last_x  = x ;
        pen . last_y = y ;
    }


    void pen_stamp ( pen_state & pen ,
                     SDL_Texture *texture ,
                     const SDL_FRect & dst ,
                     double angle_deg ,
                     SDL_FPoint center
                     ) {

        if ( !texture ) {
            return ;
        }

        PenStamp st ;
        st . texture = texture ;
        st . dst = dst ;
        st . angle_deg = angle_deg ;
        st . center = center ;

        pen . stamps . push_back ( st ) ;
    }


    void pen_render ( SDL_Renderer * ren , const pen_state & pen , const StageRect & stage ) {
        if ( !ren ) {
            return ;
        }

        for ( const auto & st : pen . stamps ) {
            if ( !st.texture ) {
                continue ;
            }
            SDL_FRect dst = st.dst ;
            ( void ) stage ;
            SDL_RenderCopyExF ( ren , st.texture , nullptr , &dst , st.angle_deg , &st.center , SDL_FLIP_NONE ) ;
        }


        for ( const auto & seg : pen . segment ) {
            float x0 = seg.x0 + ( float ) stage . x ;
            float y0 = seg.y0 + ( float ) stage . y ;
            float x1 = seg.x1 + ( float ) stage . x ;
            float y1 = seg.y1 + ( float ) stage . y ;

            draw_thick_line ( ren , x0 , y0 , x1 , y1 , seg.color , seg.size ) ;
        }
    }


}