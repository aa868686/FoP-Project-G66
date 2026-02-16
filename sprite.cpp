#include "sprite.h"
#include <algorithm>
#include <cmath>

namespace gfx {

    static float deg_to_rad ( float deg ) {
        return deg * M_PI / 180.0f ;
    }

    // Converts direction to unit vector
    // direction = 90 means facing right
    static void direction_to_unit ( float direction_deg , float & out_dx , float & out_dy ) {
        float rad = deg_to_rad ( direction_deg ) ;
        out_dx = std :: cos ( rad ) ;
        out_dy = - std :: sin ( rad ) ;
    }

    static bool has_valid_costume ( const sprite &s ) {
        if ( s . costumes. empty () ) {
            return false ;
        }
        int i = s . current_costume ;
        if ( i < 0 || i >= (int) s.costumes.size() ) {
            return false ;
        }
        const Costume &c = s.costumes[i] ;
        return c.texture != nullptr && c.tex_w > 0 && c.tex_h > 0 ;
    }


    sprite sprite_make ( int id , const char * name ) {
        sprite s ;
        s.id = id ;
        if ( name ) {
            s.name = name ;
        }

        s.visible = true ;
        s.draggable = false ;

        s.x = 0.0f ;
        s.y = 0.0f ;

        s.direction_deg = 90.0f ;
        s.size_percent = 100.0f ;

        s.current_costume = 0 ;
        s.drag = dragState {} ;

        return s ;
    }

    void sprite_set_visible ( sprite &s , bool v ) {
        s.visible = v ;
    }
    void sprite_set_draggable ( sprite &s , bool d ) {
        s.draggable = d ;
    }
    void sprite_set_position ( sprite &s , float x , float y ) {
        s.x = x ;
        s.y = y ;
    }
    void sprite_set_direction ( sprite &s , float degree ) {
        s.direction_deg = degree ;
    }

    void sprite_set_size ( sprite &s , float size_percent ) {
        s.size_percent = std :: max ( 0.0f , size_percent ) ;
    }



    bool sprite_get_render_size ( const sprite &s , float & out_w , float & out_h ) {
        if ( !has_valid_costume ( s ) ) {
            out_w = 0 ;
            out_h = 0 ;
            return false ;
        }
        const Costume &c = s.costumes [s.current_costume] ;
        float scale = s.size_percent / 100.0f ;
        out_w = (float) c.tex_w * scale ;
        out_h = (float) c.tex_h * scale ;
        return true ;
    }

    SDL_FRect sprite_get_aabb ( const sprite &s ) {
        float w = 0 , h = 0 ;
        sprite_get_render_size ( s , w , h ) ;

        SDL_FRect r{} ;
        r.w = w ;
        r.h = h ;
        r.x = s.x - ( w * 0.5f ) ;
        r.y = s.y - ( h * 0.5f ) ;
        return r ;
    }

    void sprite_move_steps ( sprite &s , float steps ) {
        float dx , dy ;
        direction_to_unit ( s.direction_deg , dx , dy ) ;
        s.x += dx * steps ;
        s.y += dy * steps ;
    }

    void sprite_turn_deg ( sprite &s , float degree ) {
        s.direction_deg += degree ;

        // Normalize angle to [0,360)
        while ( s.direction_deg >= 360.0f ) {
            s.direction_deg -= 360.0f ;
        }
        while ( s.direction_deg < 0.0f ) {
            s.direction_deg += 360.0f ;
        }
    }

    void sprite_clamp_to_stage ( sprite &s , const stage_rectangle & stage ) {
        if ( stage.w <= 0 || stage.h <= 0 ) {
            return ;
        }
        float w = 0 , h = 0 ;
        sprite_get_render_size ( s , w , h ) ;

        float half_w = w * 0.5f ;
        float half_h = h * 0.5f ;

        float min_x = (float) stage.x + half_w ;
        float max_x = (float)( stage.x + stage.w ) - half_w ;
        float min_y = (float) stage.y + half_h ;
        float max_y = (float)( stage.y + stage.h ) - half_h ;

        s.x = std :: min ( std :: max ( s.x , min_x ) , max_x ) ;
        s.y = std :: min ( std :: max ( s.y , min_y ) , max_y ) ;

    }


    ///// Costume API /////

    int sprite_add_costume ( sprite &s , SDL_Texture *tex , int tex_w , int tex_h , const char * costime_name ) {
        Costume c ;
        c . texture = tex ;
        c . tex_w = tex_w ;
        c . tex_h = tex_h ;
        if ( costime_name ) {
            c.name = costime_name ;
        }

        s.costumes.push_back ( c ) ;

        if ((int)s.costumes.size() == 1 ) {
            s.current_costume = 0 ;
        }

        return (int)s.costumes.size() - 1 ;
    }


    bool sprite_set_costume ( sprite &s , int index ) {
        if ( index < 0 || index >= (int)s.costumes.size() ) {
            return false ;
        }
        s.current_costume = index ;
        return true ;
    }

    bool sprite_set_costume_by_name ( sprite &s , const char * costume_name ) {
        if ( !costume_name ) {
            return false ;
        }

        for ( int i = 0 ; (int)s.costumes.size() ; ++i ) {
            if ( s.costumes[i].name == costume_name ) {
                s.current_costume = i ;
                return true ;
            }
        }
        return false ;
    }


    bool sprite_replace_costume_texture ( sprite &s , int index , SDL_Texture * new_tex , int new_w , int new_h ) {
        if ( index < 0 || index >= (int)s.costumes.size() ) {
            return false ;
        }
        if ( !new_tex || new_w <= 0 || new_h <= 0 ) {
            return false ;
        }

        s.costumes[index].texture = new_tex ;
        s.costumes[index].tex_w = new_w ;
        s.costumes[index].tex_h = new_h ;
        return true ;
    }


    bool sprite_replace_current_texture ( sprite &s , SDL_Texture * new_tex , int new_w , int new_h ) {
        if ( s.costumes.empty() ) {
            return false ;
        }

        return sprite_replace_costume_texture ( s , s.current_costume , new_tex , new_w , new_h ) ;
    }



    ///// Render + HitTest + Drag /////

    void sprite_draw ( SDL_Renderer *ren , const sprite &s , const stage_rectangle stage ) {
        if ( !ren || !s.visible || !has_valid_costume ( s ) ) {
            return ;
        }

        const Costume &c = s.costumes[s.current_costume] ;

        SDL_FRect aabb = sprite_get_aabb ( s ) ;

        SDL_FRect dst ;
        dst.x = aabb.x + (float) stage.x ;
        dst.y = aabb.y + (float) stage.y ;
        dst.w = aabb . w ;
        dst.h = aabb . h ;

        SDL_FPoint center { dst.w * 0.5f , dst.h * 0.5f } ;


        // Adjust angle so that 90 degrees means facing right ( no rotation visually )
        double angle = (double)(s.direction_deg - 90.0 ) ;

        SDL_RenderCopyExF ( ren , c.texture , nullptr , &dst , angle , &center , SDL_FLIP_NONE ) ;
    }


    bool sprite_hit_test_aabb ( const sprite &s , int mouse_px , int mouse_py , const stage_rectangle & stage ) {
        if ( !s.visible || !has_valid_costume ( s ) ) {
            return false ;
        }

        SDL_FRect aabb = sprite_get_aabb ( s ) ;

        float x = aabb . x + (float) stage.x ;
        float y = aabb . y + (float) stage.y ;

        return ( mouse_px >= x &&
                 mouse_px <= x + aabb.w &&
                 mouse_py >= y &&
                 mouse_py <= y + aabb.h
        ) ;
    }

    void sprite_drag_begin ( sprite &s , int mouse_px , int mouse_py , const stage_rectangle & stage ) {
        if ( !s.drag.dragging ) {
            return ;
        }

        float mx_stage = (float)( mouse_px - stage.x ) ;
        float my_stage = (float)( mouse_py - stage.y ) ;

        s.x = mx_stage + s.drag.grab_dx ;
        s.y = my_stage + s.drag.grab_dy ;

        sprite_clamp_to_stage ( s , stage ) ;
    }

    void sprite_drag_end ( sprite &s ) {
        s.drag.dragging = false ;
    }

}




































