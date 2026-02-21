#ifndef FOP_PROJECT_G66_IMAGE_EDITOR_H
#define FOP_PROJECT_G66_IMAGE_EDITOR_H

#pragma once

#include <SDL2/SDL.h>
#include "font_manager.h"
#include "sprite.h"
#include <vector>




namespace gfx {

    enum struct editor_tool {
        pen , eraser , line , circle , rect , fill
    } ;

    struct editor_color_picker {
        SDL_Rect rect {} ;
        SDL_Color selected { 0 , 0 , 0 , 255 } ;
    };

    struct image_editor {
        bool open = false ;
        SDL_Rect window_rect {} ;


        SDL_Texture * canvas = nullptr ;
         int canvas_w = 256 ;
        int canvas_h = 256 ;
        SDL_Rect canvas_rect {} ;

        editor_tool tool = editor_tool :: pen ;
        SDL_Color color { 0 , 0 , 0 , 255 } ;
        int brush_size = 3 ;

        bool drawing = false ;
        int start_x = 0 ;
        int start_y = 0 ;
        int last_x = 0 ;
        int last_y = 0 ;


        SDL_Rect toolbar {} ;
        SDL_Rect btn_pen {} ;
        SDL_Rect btn_eraser {} ;
        SDL_Rect btn_line {} ;
        SDL_Rect btn_circle {} ;
        SDL_Rect btn_rect_tool {} ;
        SDL_Rect btn_fill {} ;
        SDL_Rect btn_close {} ;
        SDL_Rect btn_apply {} ;

        editor_color_picker color_picker {} ;


        int target_sprite = -1 ;

    };


    void editor_open ( image_editor & ed ,
                       SDL_Renderer * ren ,
                       int win_w , int win_h , int sprite_idx
                       ) ;

    void editor_close ( image_editor & ed ) ;

    void editor_render ( SDL_Renderer * ren , image_editor & ed  , TTF_Font * font ) ;

    bool editor_handle_click ( image_editor & ed , int mx , int my ) ;

    void editor_handle_drag ( image_editor & ed , int mx , int my ) ;

    void editor_handle_up ( image_editor & ed , sprite_manager & sm , SDL_Renderer * ren ) ;

}

#endif //FOP_PROJECT_G66_IMAGE_EDITOR_H
