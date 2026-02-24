#include "my_block.h"
#include "font_manager.h"
#include <algorithm>
#include <cstring>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace myblock {


    int func_add ( func_store & fs , const std :: string & name ) {
        if ( name.empty() ) {
            return -1 ;
        }
        func_def f {} ;
        f.name = name ;
        fs.funcs.push_back ( f ) ;
        return (int) fs.funcs.size() - 1 ;
    }

    void func_remove ( func_store & fs , int index ) {
        if ( index < 0 || index >= (int) fs.funcs.size() ) {
            return ;
        }
        fs.funcs.erase ( fs.funcs.begin() + index ) ;
    }

    func_def * func_find ( func_store & fs , const std :: string & name ) {
        for ( auto & f : fs.funcs ) {
            if ( f.name == name ) {
                return &f ;
            }
        }
        return nullptr ;
    }

    bool func_add_param ( func_store & fs , int func_idx ,
                          const std :: string & param_name ,
                          param_type ptype ) {
        if ( func_idx < 0 || func_idx >= (int) fs.funcs.size() ) {
            return false ;
        }
        if ( param_name.empty() ) {
            return false ;
        }
        func_param p {} ;
        p.name = param_name ;
        p.type = ptype ;
        fs.funcs[func_idx].params.push_back ( p ) ;
        return true ;
    }

    bool func_name_exists ( const func_store & fs , const std :: string & name ) {
        for ( const auto & f : fs.funcs ) {
            if ( f.name == name ) {
                return true ;
            }
        }
        return false ;
    }

    std :: string func_define_label ( const func_def & f ) {
        std :: string lbl = "define " + f.name ;
        for ( const auto & p : f.params ) {
            lbl += " [" + p.name + ":" + param_type_name(p.type) + "]" ;
        }
        return lbl ;
    }

    std :: string func_call_label ( const func_def & f ) {
        std :: string lbl = f.name ;
        for ( const auto & p : f.params ) {
            lbl += " _" ;
        }
        return lbl ;
    }

    const char * param_type_name ( param_type t ) {
        switch ( t ) {
            case param_type :: type_int : return "int" ;
            case param_type :: type_float : return "float" ;
            case param_type :: type_bool : return "bool" ;
            case param_type :: type_string : return "string" ;
        }
        return "int" ;
    }


    static void fill_rect ( SDL_Renderer * ren , SDL_Rect r , Uint8 rr , Uint8 g , Uint8 b ) {
        SDL_SetRenderDrawColor ( ren , rr , g , b , 255 ) ;
        SDL_RenderFillRect ( ren , &r ) ;
    }

    static void outline_rect ( SDL_Renderer * ren , SDL_Rect r , Uint8 rr , Uint8 g , Uint8 b ) {
        SDL_SetRenderDrawColor ( ren , rr , g , b , 255 ) ;
        SDL_RenderDrawRect ( ren , &r ) ;
    }

    static bool pt_in ( SDL_Rect r , int x , int y ) {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h ;
    }

    void editor_open ( my_block_editor & ed , int win_w , int win_h ) {
        ed.open = true ;
        const int W = 480 ;
        const int H = 520 ;
        ed.window_rect = { ( win_w - W ) / 2 , ( win_h - H ) / 2 , W , H } ;
        ed.error_msg   = "" ;
        ed.editing_func = -1 ;
        std :: memset ( ed.new_name , 0 , sizeof (ed.new_name) ) ;
        std :: memset ( ed.new_param_name , 0 , sizeof (ed.new_param_name) ) ;
        ed.new_param_type = 0 ;

        const int pad = 10 ;
        const int bh  = 28 ;
        const int bw  = 120 ;
        int bx = ed.window_rect.x + pad ;
        int by = ed.window_rect.y + ed.window_rect.h - bh - pad ;

        ed.btn_confirm = { bx , by , bw , bh } ;
        ed.btn_add_param = { bx + bw + pad , by , bw , bh } ;
        ed.btn_add_func = { bx + ( ( bw + pad ) * 2 ) , by , bw , bh } ;
        ed.btn_close = { ed.window_rect.x + ed.window_rect.w - bh - pad ,
                             ed.window_rect.y + pad , bh , bh } ;
    }

    void editor_close ( my_block_editor & ed ) {
        ed.open = false ;
        ed.editing_func = -1 ;
        ed.error_msg = "" ;
    }

    void editor_render ( SDL_Renderer * ren ,
                         my_block_editor & ed ,
                         func_store & fs ,
                         TTF_Font * font ) {
        if ( !ed.open || !ren ) {
            return ;
        }

        const SDL_Rect & w = ed.window_rect ;
        const int pad = 10 ;

        fill_rect ( ren , w , 30 , 30 , 30 ) ;
        outline_rect ( ren , w , 180 , 100 , 200 ) ;

        int y = w.y + pad ;

        if ( font ) {
            SDL_Rect title_r { w.x , y , w.w , 26 } ;
            fnt :: draw_text_centered ( ren , font , "My Blocks — Define Function" , title_r , { 220 , 180 , 255 , 255 } ) ;
        }
        y += 30 ;

        if ( font ) {
            SDL_Rect lbl { w.x + pad , y , 100 , 22 } ;
            fnt :: draw_text_left ( ren , font , "Function name:" , lbl , { 200 , 200 , 200 , 255 } ) ;
        }
        y += 24 ;
        SDL_Rect name_box { w.x + pad , y , w.w - pad*2 , 26 } ;
        fill_rect ( ren , name_box , 50 , 50 , 50 ) ;
        outline_rect ( ren , name_box , 150 , 100 , 200 ) ;
        if ( font ) {
            std :: string nm = ed.new_name ;
            if ( nm.empty() ) nm = "..." ;
            fnt :: draw_text_left ( ren , font , nm.c_str() , name_box , { 255 , 255 , 255 , 255 } ) ;
        }
        y += 32 ;

        if ( font ) {
            SDL_Rect lbl { w.x + pad , y , 200 , 22 } ;
            fnt :: draw_text_left ( ren , font , "Add parameter:" , lbl , { 200 , 200 , 200 , 255 } ) ;
        }
        y += 24 ;

        SDL_Rect param_box { w.x + pad , y , w.w/2 - pad , 26 } ;
        fill_rect ( ren , param_box , 50 , 50 , 50 ) ;
        outline_rect ( ren , param_box , 100 , 100 , 180 ) ;
        if ( font ) {
            std :: string pn = ed.new_param_name ;
            if ( pn.empty() ) pn = "param name..." ;
            fnt :: draw_text_left ( ren , font , pn.c_str() , param_box , { 200 , 200 , 200 , 255 } ) ;
        }

        const char * types[] = { "int" , "float" , "bool" , "string" } ;
        int tx = w.x + w.w/2 + pad ;
        for ( int i = 0 ; i < 4 ; ++i ) {
            SDL_Rect tb { tx + i*55 , y , 50 , 26 } ;
            if ( ed.new_param_type == i ) {
                fill_rect( ren , tb , 80 , 40 , 160 ) ;
            } else {
                fill_rect ( ren , tb , 55 , 55 , 55 ) ;
            }
            outline_rect ( ren , tb , 100 , 100 , 150 ) ;
            if ( font ) {
                fnt::draw_text_centered(ren, font, types[i], tb, {220, 220, 220, 255});
            }
        }
        y += 34 ;

        if ( font ) {
            SDL_Rect lbl { w.x + pad , y , 200 , 22 } ;
            fnt :: draw_text_left ( ren , font , "Defined functions:" , lbl , { 200,200,200,255 } ) ;
        }
        y += 26 ;

        for ( int i = 0 ; i < (int) fs.funcs.size() && y < w.y + w.h - 60 ; ++i ) {
            const auto & f = fs.funcs[i] ;
            SDL_Rect row { w.x + pad , y , w.w - pad*2 , 24 } ;
            if ( i == ed.editing_func ) {
                fill_rect ( ren , row , 60 , 40 , 100 ) ;
            } else {
                fill_rect ( ren , row , 40 , 40 , 40 ) ;
            }
            outline_rect ( ren , row , 100 , 80 , 150 ) ;

            std :: string info = func_define_label ( f ) ;

            if ( font ) {
                fnt::draw_text_left(ren, font, info.c_str(), row, {220, 220, 255, 255});
            }

            SDL_Rect del { w.x + w.w - pad - 22 , y + 2 , 20 , 20 } ;
            fill_rect    ( ren , del , 160 , 40 , 40 ) ;
            outline_rect ( ren , del , 80 , 80 , 80 ) ;
            if ( font ) fnt :: draw_text_centered ( ren , font , "X" , del , { 255,255,255,255 } ) ;

            y += 28 ;
        }

        if ( !ed.error_msg.empty() && font ) {
            SDL_Rect er { w.x + pad , w.y + w.h - 80 , w.w - ( pad * 2 ) , 20 } ;
            fnt :: draw_text_left ( ren , font , ed.error_msg.c_str() , er , { 255 , 100 , 100 , 255 } ) ;
        }

        fill_rect ( ren , ed.btn_confirm , 40  , 150 , 60  ) ; outline_rect( ren , ed.btn_confirm , 80,80,80 ) ;
        fill_rect ( ren , ed.btn_add_param , 60  , 60  , 160 ) ; outline_rect( ren , ed.btn_add_param , 80,80,80 ) ;
        fill_rect ( ren , ed.btn_add_func , 100 , 60  , 60  ) ; outline_rect( ren , ed.btn_add_func , 80,80,80 ) ;
        fill_rect ( ren , ed.btn_close , 160 , 40  , 40  ) ; outline_rect( ren , ed.btn_close , 80,80,80 ) ;

        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Make Block" , ed.btn_confirm , {255,255,255,255} ) ;
            fnt :: draw_text_centered ( ren , font , "Add Param" , ed.btn_add_param , {255,255,255,255} ) ;
            fnt :: draw_text_centered ( ren , font , "Delete Func" , ed.btn_add_func , {255,255,255,255} ) ;
            fnt :: draw_text_centered ( ren , font , "X" , ed.btn_close , {255,255,255,255} ) ;
        }
    }

    bool editor_handle_click ( my_block_editor & ed ,
                               func_store & fs ,
                               int mx , int my ,
                               int & out_new_func ) {
        out_new_func = -1 ;
        if ( !ed.open ) {
            return false ;
        }
        if ( !pt_in ( ed.window_rect , mx , my ) ) {
            return false ;
        }


        if ( pt_in ( ed.btn_close , mx , my ) ) {
            editor_close ( ed ) ;
            return true ;
        }

        if ( pt_in ( ed.btn_confirm , mx , my ) ) {
            std :: string name = ed.new_name ;
            if ( name.empty() ) {
                ed.error_msg = "Error: Function name is empty!" ;
                return true ;
            }
            if ( func_name_exists ( fs , name ) ) {
                ed.error_msg = "Error: Function name already exists!" ;
                return true ;
            }
            int idx = func_add ( fs , name ) ;
            ed.editing_func = idx ;
            ed.error_msg = "" ;
            out_new_func = idx ;
            std :: memset ( ed.new_name , 0 , sizeof(ed.new_name) ) ;
            return true ;
        }

        if ( pt_in ( ed.btn_add_param , mx , my ) ) {
            if ( ed.editing_func < 0 ) {
                ed.error_msg = "First create a function with Make Block!" ;
                return true ;
            }
            std :: string pname = ed.new_param_name ;
            if ( pname.empty() ) {
                ed.error_msg = "Error: Parameter name is empty!" ;
                return true ;
            }

            param_type pt = static_cast<param_type> ( ed.new_param_type ) ;
            func_add_param ( fs , ed.editing_func , pname , pt ) ;
            ed.error_msg = "" ;
            std :: memset ( ed.new_param_name , 0 , sizeof(ed.new_param_name) ) ;
            return true ;
        }

        if ( pt_in ( ed.btn_add_func , mx , my ) ) {
            if ( ed.editing_func >= 0 ) {
                func_remove ( fs , ed.editing_func ) ;
                ed.editing_func = -1 ;
                ed.error_msg = "" ;
            } else {
                ed.error_msg = "Select a function to delete." ;
            }
            return true ;
        }

        const SDL_Rect & w = ed.window_rect ;
        const int pad = 10 ;

        int tx = w.x + w.w/2 + pad ;
        int ty = w.y + pad + 30 + 24 + 24 + 24 ;
        for ( int i = 0 ; i < 4 ; ++i ) {
            SDL_Rect tb { tx + ( i * 55 ) , ty , 50 , 26 } ;
            if ( pt_in ( tb , mx , my ) ) {
                ed.new_param_type = i ;
                return true ;
            }
        }

        int ly = w.y + pad + 30 + 24 + 24 + 34 + 26 ;
        for ( int i = 0 ; i < (int) fs.funcs.size() ; ++i ) {
            SDL_Rect row { w.x + pad , ly , w.w - ( pad * 2 ) , 24 } ;
            SDL_Rect del { w.x + w.w - pad - 22 , ly + 2 , 20 , 20 } ;

            if ( pt_in ( del , mx , my ) ) {
                func_remove ( fs , i ) ;
                if ( ed.editing_func == i ) {
                    ed.editing_func = -1 ;
                }
                else if ( ed.editing_func > i ) {
                    ed.editing_func-- ;
                }
                return true ;
            }
            if ( pt_in ( row , mx , my ) ) {
                ed.editing_func = i ;
                return true ;
            }
            ly += 28 ;
        }

        return true ;
    }

    void editor_handle_text ( my_block_editor & ed , const char * text ) {
        if ( !ed.open || !text ) {
            return ;
        }

        if ( ed.editing_func < 0 ) {
            size_t len = std :: strlen ( ed.new_name ) ;
            if ( len + 1 < sizeof(ed.new_name) ) {
                ed.new_name[len] = text[0] ;
                ed.new_name[len+1] = '\0' ;
            }
        } else {
            size_t len = std :: strlen ( ed.new_param_name ) ;
            if ( len + 1 < sizeof(ed.new_param_name) ) {
                ed.new_param_name[len] = text[0] ;
                ed.new_param_name[len+1] = '\0' ;
            }
        }
    }

    void editor_handle_backspace ( my_block_editor & ed ) {
        if ( !ed.open ) {
            return ;
        }
        if ( ed.editing_func < 0 ) {
            size_t len = std :: strlen ( ed.new_name ) ;
            if ( len > 0 ) ed.new_name[len-1] = '\0' ;
        } else {
            size_t len = std :: strlen ( ed.new_param_name ) ;
            if ( len > 0 ) ed.new_param_name[len-1] = '\0' ;
        }
    }

}
