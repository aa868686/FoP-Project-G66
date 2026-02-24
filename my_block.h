#ifndef FOP_PROJECT_G66_MY_BLOCK_H
#define FOP_PROJECT_G66_MY_BLOCK_H

#pragma once

#include <string>
#include <vector>
#include "value.h"
#include "ui_block.h"

namespace myblock {

    enum struct param_type {
        type_int ,
        type_float ,
        type_bool ,
        type_string
    };

    struct func_param {
        std :: string name {} ;
        param_type type = param_type :: type_int ;
    };

    struct func_def {
        std :: string name {} ;
        std :: vector <func_param> params {} ;
        std :: vector <int> body_ids {} ;
    };

    struct func_store {
        std :: vector <func_def> funcs {} ;
    };

    int func_add ( func_store & fs , const std :: string & name ) ;

    void func_remove ( func_store & fs , int index ) ;

    func_def * func_find ( func_store & fs , const std :: string & name ) ;

    bool func_add_param ( func_store & fs , int func_idx ,
                          const std :: string & param_name ,
                          param_type ptype
                          ) ;

    bool func_name_exists ( const func_store & fs , const std :: string & name ) ;

    std :: string func_define_label ( const func_def & f ) ;

    std :: string func_call_label ( const func_def & f ) ;

    const char * param_type_name ( param_type t ) ;

    struct my_block_editor {
        bool open = false ;

        char new_name[64] = {} ;
        char new_param_name[64] = {} ;
        int  new_param_type = 0  ;
        std :: string error_msg {} ;

        int editing_func = -1 ;

        SDL_Rect window_rect {} ;
        SDL_Rect btn_close {} ;
        SDL_Rect btn_add_func {} ;
        SDL_Rect btn_add_param {} ;
        SDL_Rect btn_confirm {} ;
    };


    void editor_open  ( my_block_editor & ed , int win_w , int win_h ) ;

    void editor_close ( my_block_editor & ed ) ;

    void editor_render ( SDL_Renderer * ren ,
                         my_block_editor & ed ,
                         func_store & fs ,
                         TTF_Font * font ) ;


    bool editor_handle_click ( my_block_editor & ed ,
                               func_store & fs ,
                               int mx , int my ,
                               int & out_new_func ) ;

    void editor_handle_text ( my_block_editor & ed , const char * text ) ;

    void editor_handle_backspace ( my_block_editor & ed ) ;

}

#endif //FOP_PROJECT_G66_MY_BLOCK_H
