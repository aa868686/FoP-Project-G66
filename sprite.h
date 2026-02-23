#ifndef PROJECT_SPRITE_H
#define PROJECT_SPRITE_H

#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

namespace gfx {

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

        float x = 0.0f ;
        float y = 0.0f ;


        float direction_deg = 90.0f ;

        float size_percent = 100.0f ;

        std :: vector < Costume > costumes ;
        int current_costume = 0 ;

        dragState drag ;

        std::string say_text = "";
        bool say_visible = false;
        bool think_bubble = false;
    };



    struct stage_rectangle {
        int x = 0 ;
        int y = 0 ;
        int w = 0 ;
        int h = 0 ;
    };

    struct sprite_manager {
        std :: vector < sprite > sprites {} ;
        int active = -1 ;
    };


    int sprite_manager_add ( sprite_manager & sm , sprite s ) ;
    void sprite_manager_remove ( sprite_manager & sm , int index ) ;
    void sprite_manager_select ( sprite_manager & sm , int index ) ;

    void sprite_manager_render ( SDL_Renderer * ren ,
                                 sprite_manager & sm ,
                                 SDL_Rect panel ,
                                 TTF_Font * font
                                 ) ;

    bool sprite_manager_handle_click ( sprite_manager & sm , SDL_Rect panel , int mx , int my ) ;



    sprite sprite_make ( int id , const char *name ) ;

    void sprite_set_visible ( sprite &s , bool v ) ;
    void sprite_set_draggable ( sprite &s , bool d ) ;
    void sprite_set_position ( sprite &s , float x , float y ) ;
    void sprite_set_direction ( sprite &s , float deg ) ;
    void sprite_set_size ( sprite &s , float size_percent ) ;

    bool sprite_get_render_size ( const sprite &s , float &out_w , float &out_h ) ;


    SDL_FRect sprite_get_aabb ( const sprite &s ) ;

    void sprite_move_steps ( sprite &s , float steps ) ;
    void sprite_turn_degree ( sprite &S , float deg ) ;


    void sprite_clamp_to_stage ( sprite &s , const stage_rectangle &stage ) ;



    int sprite_add_costume ( sprite &s , SDL_Texture *tex , int tex_w , int tex_h , const char * costume_name ) ;


    bool sprite_set_costume ( sprite &s , int index ) ;
    bool sprite_set_costume_by_name ( sprite &s , const char * costume_name ) ;



    bool sprite_replace_costume_texture ( sprite &s , int index , SDL_Texture *new_tex , int new_w , int new_h ) ;



    bool sprite_replace_current_texture ( sprite &s , SDL_Texture * new_tex , int new_w , int new_h ) ;




    void sprite_draw(SDL_Renderer *ren, const sprite &s, const stage_rectangle &stage, TTF_Font* font = nullptr);

    bool sprite_hit_test_aabb ( const sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;


    void sprite_drag_begin ( sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;
    void sprite_drag_update ( sprite &s , int mouse_px , int mouse_py , const stage_rectangle &stage ) ;
    void sprite_drag_end ( sprite &s ) ;

}




#endif //PROJECT_SPRITE_H
