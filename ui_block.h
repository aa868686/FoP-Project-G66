#ifndef FOP_PROJECT_G66_UI_BLOCK_H
#define FOP_PROJECT_G66_UI_BLOCK_H

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>

namespace ui {

    enum struct block_category {
        motion , looks , control , events , operators , sound , variables , sensing ,
    } ;

    struct block_palette_state {
        block_category selected_category = block_category :: motion ;
    };

    struct block_input {
        std :: string value = "10" ;
        bool editable = true ;
        SDL_Rect rect {} ;
        bool focused = false ;
        int cursor_pos = 0 ;
        int sel_start = -1 ;
        int sel_end = -1 ;
    };


    struct ui_block {
        int id = -1 ;
        std :: string label {} ;
        block_category category = block_category :: motion ;


        int x = 0 ;
        int y = 0 ;
        int w = 160 ;
        int h = 36 ;

        std :: vector <block_input> inputs {} ;

        int snap_to = -1 ;

        bool dragging = false ;
        int drag_dx = 0 ;
        int drag_dy = 0 ;

        bool is_container = false ;
        std :: vector <int> children {} ;
        int container_h = 48 ;
        int parent_id = -1 ;
    };

    struct custom_block_def {
        int id = -1 ;
        std :: string name {} ;
    };



    struct block_workspace {

        std :: vector <ui_block> blocks {} ;
        std :: vector <custom_block_def> custom_blocks {} ;
        int next_custom_id = 0 ;
        int next_id = 0 ;

        int scroll_x = 0 ;
        int scroll_y = 0 ;

        int focused_block = -1 ;
        int focused_input = -1 ;

        int drag_idx = -1 ;
    };

    SDL_Color block_category_color ( block_category cat ) ;

    ui_block block_make ( int id , const std :: string & label ,
                          block_category cat ,
                          int x , int y
                          ) ;

    int block_workspace_add ( block_workspace & ws ,
                              const std :: string & label ,
                              block_category cat ,
                              int x , int y
                              ) ;

    void block_workspace_remove ( block_workspace & ws , int id ) ;

    void block_workspace_render ( SDL_Renderer * ren ,
                                  block_workspace & ws ,
                                  SDL_Rect clip ,
                                  TTF_Font * font
                                  ) ;


    int block_hit_test ( const block_workspace & ws ,
                         SDL_Rect clip ,
                         int mx , int my
                         ) ;

    void block_drag_begin ( block_workspace & ws , int idx , int mx , int my ) ;
    void block_drag_update ( block_workspace & ws , int mx , int my ) ;
    void block_drag_end ( block_workspace & ws ) ;

    bool block_input_handle_click ( block_workspace & ws , SDL_Rect clip , int mx , int my ) ;
    void block_input_handle_key ( block_workspace & ws , SDL_Keycode key , const char * text ) ;


    void block_try_snap ( block_workspace & ws , int idx ) ;


    void block_palette_render ( SDL_Renderer * ren ,
                                SDL_Rect panel ,
                                TTF_Font * font ,
                                block_palette_state & state ,
                                const block_workspace & ws
                                ) ;


    bool block_palette_handle_click ( SDL_Rect panel , int mx , int my , block_palette_state & state ) ;


    bool block_palette_click ( SDL_Rect panel ,
                               int mx , int my ,
                               block_category & out_cat ,
                               std :: string & out_label ,
                               const block_palette_state & state
                               ) ;

    void custom_block_add ( block_workspace & ws , const char * name ) ;

    void custom_block_remove ( block_workspace & ws , int id ) ;


}

#endif //FOP_PROJECT_G66_UI_BLOCK_H
