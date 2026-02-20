#ifndef FOP_PROJECT_G66_DEBUG_LOGGER_H
#define FOP_PROJECT_G66_DEBUG_LOGGER_H

#pragma once

#include <SDL2/SDL.h>
#include "font_manager.h"
#include <string>
#include <vector>

namespace dbg {

    enum struct log_level {
        info , warn , error
    };

    struct log_entry {
        std :: string text {} ;
        log_level level = log_level :: info ;
    };

    struct debug_logger {
        bool open = false ;

        std :: vector <log_entry> entries {} ;

        SDL_Rect window_rect {} ;

        SDL_Rect log_area {} ;
        SDL_Rect btn_clear {} ;
        SDL_Rect btn_close {} ;

        int scroll_offset = 0 ;
        bool auto_scroll = true ;

        static constexpr int max_entries = 200 ;
    } ;


    void logger_log (debug_logger & lg ,
                     const std :: string & text ,
                     log_level level = log_level :: info
                     ) ;

    void logger_warn ( debug_logger & lg , const std :: string & text ) ;
    void logger_error ( debug_logger & lg , const std :: string & text ) ;
    void logger_clear ( debug_logger & lg ) ;

    void logger_open ( debug_logger & lg , int win_W , int win_h ) ;
    void logger_close ( debug_logger & lg ) ;

    void logger_render ( SDL_Renderer * ren , debug_logger & lg , TTF_Font * font ) ;


    bool logger_handle_click ( debug_logger & lg , int mx , int my ) ;
    void logger_handle_scroll ( debug_logger & lg , int dy ) ;


}

#endif //FOP_PROJECT_G66_DEBUG_LOGGER_H
