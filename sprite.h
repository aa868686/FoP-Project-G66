#ifndef PROJECT_SPRITE_H
#define PROJECT_SPRITE_H

#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <SDL_image.h>

namespace gfx {

    // Representing a single sprite image ( costume ) .
    struct Costume {
        SDL_Texture * texture = nullptr ;
        int tex_w = 0 ;
        int tex_h = 0 ;
        std :: string name ;
    };

    struct dragState {
        bool dragging = false ;

        float grab_dx = 0.0f ;
        float grab_dy = 0.0f ;
    };

    struct sprite {
        int id = -1 ;
        std :: string name ;

        bool visible = true ;
        bool draggable = false ;

        //Sprite center position in stage coordinates ( pixels )
        float x = 0.0f ;
        float y = 0.0f ;


        float direction_deg = 90.0f ;

        float size_percent = 100.0f ;   // original size = 100

        std :: vector < Costume > costumes ;
        int current_costume = 0 ;

        dragState drag ;
    };



    // Defines the stage rectangle inside the window
    struct stage_rectangle {
        int x = 0 ;
        int y = 0 ;
        int w = 0 ;
        int h = 0 ;
    };



    // Creation and configuration
    sprite sprite_make ( int id , const char *name ) ;

    void sprite_set_visible ( sprite &s , bool v ) ;
    void sprite_set_draggable ( sprite &s , bool d ) ;
    void sprite_set_position ( sprite &s , float x , float y ) ;
    void sprite_set_direction ( sprite &s , float deg ) ;
    void sprite_set_size ( sprite &s , float size_percent ) ;

    // Returns scaled render size
    bool sprite_get_render_size ( const sprite &s , float &out_w , float &out_h ) ;


    // Axis-aligned bounding box ( ignores rotation )
    SDL_FRect sprite_get_aabb ( const sprite &s ) ;

    // Movement and rotation
    void sprite_move_steps ( sprite &s , float steps ) ;
    void sprite_turn_degree ( sprite &S , float deg ) ;


    // Keeps sprite inside stage bounds ( aabb-based , no rotation )
    void sprite_clamp_to_stage ( sprite &s , const stage_rectangle &stage ) ;


    ////// Costume API /////

    //Adds a new costume
    int sprite_add_costume ( sprite &s , SDL_Texture *tex , int tex_w , int tex_h , const char * costume_name ) ;

    // Switch costume by index or name
    bool sprite_set_costume ( sprite &s , int index ) ;
    bool sprite_set_costume_by_name ( sprite &s , const char * costume_name ) ;


    // Replace texture of an existing costume
    bool sprite_replace_costume_texture ( sprite &s , int index , SDL_Texture *new_tex , int new_w , int new_h ) ;


    // Replace texture of current costume
    bool sprite_replace_current_texture ( sprite &s , SDL_Texture * new_tex , int new_w , int new_h ) ;



    ///// Render + HitTest + Dragging /////


    void sprite_draw ( SDL_Renderer *ren , const sprite &s , const stage_rectangle &stage ) ;


    // aabb hit test
    bool sprite_hit_test_aabb ( const sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;


    // Drag handling
    void sprite_drag_begin ( sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;
    void sprite_drag_update ( sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;
    void sprite_drag_end ( sprite &s ) ;

}




#endif //PROJECT_SPRITE_H
