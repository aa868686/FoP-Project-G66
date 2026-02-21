#include "debug_logger.h"
#include <algorithm>

namespace dbg {

    static constexpr int dl_pad = 8 ;
    static constexpr int dl_btn_w = 70 ;
    static constexpr int dl_btn_h = 28 ;
    static constexpr int dl_header_h = 44 ;
    static constexpr int dl_line_h = 18 ;


    static void dl_fill ( SDL_Renderer * ren , SDL_Rect r ,
                          Uint8 rr , Uint8 g , Uint8 b , Uint8 a = 255
                          ) {
        SDL_SetRenderDrawBlendMode ( ren ,
            a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE ) ;
        SDL_SetRenderDrawColor ( ren , rr , g ,b ,a ) ;
        SDL_RenderFillRect ( ren , &r ) ;

    }

    static void dl_outline ( SDL_Renderer * ren , SDL_Rect r ,
                             Uint8 rr  , Uint8 g , Uint8 b
                             ) {
        SDL_SetRenderDrawColor ( ren , rr , g , b , 255 ) ;
        SDL_RenderDrawRect ( ren , &r ) ;
    }

    static bool dl_hit (  SDL_Rect r , int x , int y ) {
        return x >= r.x && x < r.x + r.w &&
               y >= r.y && y < r.y + r.h ;
    }

    static SDL_Color level_color ( log_level lv ) {
        switch ( lv ) {
            case log_level :: info : return { 200 , 200 , 200 , 255 } ;
            case log_level :: warn : return { 255 , 200 , 50 , 255 } ;
            case log_level :: error : return { 255 , 80 , 80 , 255 } ;
        }
        return { 200 , 200 , 200 , 255 } ;
    }


    static void build_rects ( debug_logger & lg ) {
        const SDL_Rect & w = lg.window_rect ;
        const int by = w.y + ( ( dl_header_h - dl_btn_h ) / 2 ) ;

        lg.btn_close = {w.x + w.w - dl_pad - dl_btn_w,
                        by, dl_btn_w, dl_btn_h
        } ;

        lg.btn_clear = {
                w.x + w.w - ( dl_pad * 2 ) - ( dl_btn_w * 2 ) ,
                by , dl_btn_w , dl_btn_h
        } ;

        lg.log_area = { w.x + dl_pad ,
                        w.y + dl_header_h + dl_pad ,
                        w.w - ( dl_pad * 2 ) ,
                        w.h - dl_header_h - ( dl_pad * 2 )
        } ;
    }



    void logger_log ( debug_logger & lg , const std :: string & text , log_level level ) {
        log_entry e {} ;
        e.text = text ;
        e.level = level ;
        lg.entries.push_back ( e ) ;


        if ( static_cast <int> ( lg.entries.size() ) > debug_logger :: max_entries ) {
            lg.entries.erase ( lg.entries.begin() ) ;
        }

        if ( lg.auto_scroll ) {
            lg.scroll_offset = 0 ;
        }
    }

    void logger_warn  ( debug_logger & lg , const std :: string & text ) {
        logger_log ( lg , text , log_level :: warn ) ;
    }

    void logger_error ( debug_logger & lg , const std :: string & text ) {
        logger_log ( lg , text , log_level :: error ) ;
    }

    void logger_clear ( debug_logger & lg ) {
        lg.entries.clear() ;
        lg.scroll_offset = 0 ;
    }



    void logger_open ( debug_logger & lg , int win_w , int win_h ) {
        const int lw = std :: min ( 700 , win_w - 40 ) ;
        const int lh = std :: min ( 400 , win_h - 40 ) ;
        lg.window_rect = {
                ( win_w - lw ) / 2 ,
                ( win_h - lh ) / 2 ,
                lw , lh
        } ;
        build_rects ( lg ) ;
        lg.open = true ;
    }

    void logger_close ( debug_logger & lg ) {
        lg.open = false ;
    }



    void logger_render ( SDL_Renderer * ren ,
                         debug_logger & lg ,
                         TTF_Font * font ) {
        if ( !lg.open || !ren ) {
            return ;
        }


        SDL_SetRenderDrawBlendMode ( ren , SDL_BLENDMODE_BLEND ) ;
        SDL_SetRenderDrawColor ( ren , 0 , 0 , 0 , 140 ) ;
        SDL_Rect full { 0 , 0 , 4000 , 4000 } ;
        SDL_RenderFillRect ( ren , &full ) ;

        dl_fill    ( ren , lg.window_rect , 22 , 22 , 22 ) ;
        dl_outline ( ren , lg.window_rect , 70 , 70 , 70 ) ;


        SDL_Rect header { lg.window_rect.x , lg.window_rect.y , lg.window_rect.w , dl_header_h } ;

        dl_fill    ( ren , header , 32 , 32 , 32 ) ;
        dl_outline ( ren , header , 60 , 60 , 60 ) ;



        if ( font ) {
            SDL_Rect title_r { lg.window_rect.x + dl_pad ,
                               lg.window_rect.y ,
                               200 , dl_header_h } ;
            fnt :: draw_text_left ( ren , font , "Debug Logger" ,
                                  title_r , { 180,180,180,255 } ) ;
        }


        dl_fill    ( ren , lg.btn_clear , 60  , 60  , 60  ) ;
        dl_outline ( ren , lg.btn_clear , 100 , 100 , 100 ) ;
        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Clear" ,
                                      lg.btn_clear , { 220,220,220,255 } ) ;
        }


        dl_fill    ( ren , lg.btn_close , 160 , 40  , 40  ) ;
        dl_outline ( ren , lg.btn_close , 80  , 80  , 80  ) ;
        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Close" ,
                                      lg.btn_close , { 255,255,255,255 } ) ;
        }


        dl_fill    ( ren , lg.log_area , 15 , 15 , 15 ) ;
        dl_outline ( ren , lg.log_area , 50 , 50 , 50 ) ;

        if ( !font ) {
            return ;
        }


        SDL_RenderSetClipRect ( ren , &lg.log_area ) ;

        const int visible_lines = lg.log_area.h / dl_line_h ;
        const int total = static_cast <int> ( lg.entries.size() ) ;


        const int max_scroll = std :: max ( 0 , total - visible_lines ) ;
        lg.scroll_offset = std :: min ( lg.scroll_offset , max_scroll ) ;
        const int start_idx = std :: max ( 0 , total - visible_lines - lg.scroll_offset ) ;

        for ( int i = start_idx ; i < total ; ++i ) {
            const log_entry & e = lg.entries[i] ;
            const int line_y = lg.log_area.y + ( dl_pad / 2 ) + ( ( i - start_idx ) * dl_line_h ) ;


            SDL_Color lc = level_color ( e.level ) ;
            SDL_Rect bar { lg.log_area.x , line_y , 3 , dl_line_h - 2 } ;
            dl_fill ( ren , bar , lc.r , lc.g , lc.b ) ;


            SDL_Rect text_r {
                    lg.log_area.x + dl_pad ,
                    line_y ,
                    lg.log_area.w - dl_pad ,
                    dl_line_h
            } ;
            fnt :: draw_text_left ( ren , font , e.text.c_str() , text_r , lc ) ;
        }

        SDL_RenderSetClipRect ( ren , nullptr ) ;


        if ( total > visible_lines ) {
            const float ratio = 1.0f - ( static_cast <float> ( lg.scroll_offset ) / static_cast <float> ( total - visible_lines ) ) ;
            const int bar_h = std :: max ( 20 , lg.log_area.h * visible_lines / total ) ;
            const int bar_y = lg.log_area.y + static_cast <int> ( ratio * ( lg.log_area.h - bar_h ) ) ;
            SDL_Rect scroll_bar {
                    lg.log_area.x + lg.log_area.w - 6 ,
                    bar_y , 4 , bar_h
            } ;
            dl_fill ( ren , scroll_bar , 80 , 80 , 80 ) ;
        }
    }


    bool logger_handle_click ( debug_logger & lg , int mx , int my ) {
        if ( !lg.open ) {
            return false ;
        }
        if ( !dl_hit ( lg.window_rect , mx , my ) ) {
            return false ;
        }

        if ( dl_hit ( lg.btn_close , mx , my ) ) {
            logger_close ( lg ) ;
            return true ;
        }

        if ( dl_hit ( lg.btn_clear , mx , my ) ) {
            logger_clear ( lg ) ;
            return true ;
        }


        if ( dl_hit ( lg.log_area , mx , my ) ) {
            lg.auto_scroll = false ;
            return true ;
        }

        return true ;
    }

    void logger_handle_scroll ( debug_logger & lg , int dy ) {
        if ( !lg.open ) {
            return ;
        }

        lg.scroll_offset += dy ;
        lg.scroll_offset  = std :: max ( 0 , lg.scroll_offset ) ;


        const int visible_lines = lg.log_area.h / dl_line_h ;
        const int total = static_cast <int> ( lg.entries.size() ) ;
        const int max_scroll = std :: max ( 0 , total - visible_lines ) ;

        if ( lg.scroll_offset >= max_scroll ) {
            lg.scroll_offset = max_scroll ;
            lg.auto_scroll = true ;
        } else {
            lg.auto_scroll = false ;
        }
    }

}
