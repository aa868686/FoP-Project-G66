#include "ui_block.h"
#include "font_manager.h"
#include <algorithm>
#include <cstring>

namespace ui {


    struct palette_entry {
        block_category cat ;
        const char * label ;
    } ;

    static const palette_entry PALETTE[] = {

            { block_category :: motion , "move _ steps" } ,
            { block_category :: motion , "turn _ degrees" } ,
            { block_category :: motion , "go to x:_ y:_" } ,
            { block_category :: motion , "point in direction _" } ,
            { block_category :: motion , "if on edge bounce" } ,


            { block_category :: looks , "say _ for _ secs" } ,
            { block_category :: looks , "say _" } ,
            { block_category :: looks , "think _ for _ secs" } ,
            { block_category :: looks , "show" } ,
            { block_category :: looks , "hide" } ,
            { block_category :: looks , "set size to _%" } ,


            { block_category :: events , "when flag clicked" } ,
            { block_category :: events , "when key _ pressed" } ,
            { block_category :: events , "when sprite clicked" } ,
            { block_category :: events , "broadcast _" } ,


            { block_category :: control , "wait _ secs" } ,
            { block_category :: control , "repeat _" } ,
            { block_category :: control , "forever" } ,
            { block_category :: control , "if _ then" } ,
            { block_category :: control , "if _ then else" } ,
            { block_category :: control , "stop all" } ,


            { block_category :: operators , "_ + _" } ,
            { block_category :: operators , "_ - _" } ,
            { block_category :: operators , "_ * _" } ,
            { block_category :: operators , "_ / _" } ,
            { block_category :: operators , "_ = _" } ,
            { block_category :: operators , "_ < _" } ,
            { block_category :: operators , "_ > _" } ,
            { block_category :: operators , "_ and _" } ,
            { block_category :: operators , "_ or _" } ,
            { block_category :: operators , "not _" } ,

            { block_category :: sound , "play sound _" } ,
            { block_category :: sound , "stop all sounds" } ,
            { block_category :: sound , "set volume to _%"} ,
            { block_category :: sound , "change volume by _" } ,
    } ;

    static constexpr int palette_count = static_cast <int> ( sizeof(PALETTE) / sizeof ( PALETTE[0] ) ) ;

    static constexpr int block_h       = 36 ;
    static constexpr int block_w       = 200 ;
    static constexpr int snap_dist     = 18 ;
    static constexpr int pad           = 6  ;



    SDL_Color block_category_color ( block_category cat ) {
        switch ( cat ) {
            case block_category :: motion : return { 70  , 130 , 210 , 255 } ;
            case block_category :: looks : return { 100 , 80  , 200 , 255 } ;
            case block_category :: control : return { 220 , 180 , 20  , 255 } ;
            case block_category :: events : return { 200 , 130 , 20  , 255 } ;
            case block_category :: operators : return { 60  , 170 , 80  , 255 } ;
            case block_category :: sound : return { 180 , 80 , 200 , 255 } ;
        }
        return { 100 , 100 , 100 , 255 } ;
    }

    static SDL_Color darker ( SDL_Color c , int by = 30 ) {
        auto clamp = [] ( int v ) -> Uint8 {
            return static_cast <Uint8> ( std :: max ( 0 , std :: min ( 255 , v ) ) ) ;
        } ;
        return { clamp (c.r - by) , clamp (c.g - by) , clamp (c.b - by) , 255 } ;
    }


    ui_block block_make ( int id ,
                          const std :: string & label ,
                          block_category cat ,
                          int x , int y ) {
        ui_block b {} ;
        b.id = id ;
        b.label = label ;
        b.category = cat ;
        b.x = x ;
        b.y = y ;
        b.w = block_w ;
        b.h = block_h ;

        if ( label.find ( "_") != std :: string :: npos ) {
            block_input inp {} ;
            inp.value = "10" ;
            b.inputs.push_back ( inp ) ;
        }

        size_t first = label.find ( "_" ) ;
        size_t second = label.find ( "_" , first + 1 ) ;
        if ( first != std :: string :: npos && second != std :: string :: npos ) {
            block_input inp2 {} ;
            inp2.value = "0" ;
            b.inputs.push_back ( inp2 ) ;
        }
        return b ;
    }

    int block_workspace_add ( block_workspace & ws ,
                              const std :: string & label ,
                              block_category cat ,
                              int x , int y
                              ) {
        ui_block b = block_make ( ws.next_id++ , label , cat , x , y ) ;
        ws.blocks.push_back ( b ) ;
        return static_cast <int> ( ws.blocks.size() ) - 1 ;
    }

    void block_workspace_remove ( block_workspace & ws , int id ) {
        ws.blocks.erase (
                std :: remove_if ( ws.blocks.begin() , ws.blocks.end() ,
                                 [id]( const ui_block & b ){ return b.id == id ; } ) ,
                ws.blocks.end() ) ;
    }


    static void fill_rect ( SDL_Renderer * ren , SDL_Rect r , SDL_Color c ) {
        SDL_SetRenderDrawColor ( ren , c.r , c.g , c.b , c.a ) ;
        SDL_RenderFillRect ( ren , &r ) ;
    }

    static void draw_rect_outline ( SDL_Renderer * ren , SDL_Rect r , SDL_Color c ) {
        SDL_SetRenderDrawColor ( ren , c.r , c.g , c.b , c.a ) ;
        SDL_RenderDrawRect ( ren , &r ) ;
    }


    static void draw_notch ( SDL_Renderer * ren , SDL_Rect block_r , SDL_Color c ) {

        SDL_Rect notch {
                block_r.x + ( block_r.w / 2 ) - 10 ,
                block_r.y - 4 ,
                20 , 5
        } ;

        SDL_Color nc = darker ( c , 40 ) ;
        SDL_SetRenderDrawColor ( ren , nc.r , nc.g , nc.b , 255 ) ;
        SDL_RenderFillRect ( ren , &notch ) ;
    }


    static void draw_block ( SDL_Renderer * ren ,
                             const ui_block & b ,
                             int sx , int sy ,
                             TTF_Font * font
                             ) {
        SDL_Rect r { sx , sy , b.w , b.h } ;

        SDL_Color col  = block_category_color ( b.category ) ;
        SDL_Color dark = darker ( col ) ;

        SDL_Rect shadow { r.x + 2 , r.y + 2 , r.w , r.h } ;
        SDL_SetRenderDrawColor ( ren , 0 , 0 , 0 , 80 ) ;
        SDL_RenderFillRect ( ren , &shadow ) ;

        fill_rect ( ren , r , col ) ;

        SDL_Rect bottom { r.x , r.y + r.h - 4 , r.w , 4 } ;
        fill_rect ( ren , bottom , dark ) ;

        draw_rect_outline ( ren , r , darker ( col , 50 ) ) ;


        draw_notch ( ren , r , col ) ;

        if ( !font ) {
            return ;
        }

        std :: string lbl = b.label ;
        int draw_x = sx + pad ;
        const int inp_h = 20 ;
        const int inp_w = 36 ;
        const int text_y = sy + ( ( b.h - inp_h ) / 2 ) ;
        int input_idx = 0 ;
        size_t pos = 0 ;

        while ( pos <= lbl.size() ) {
            size_t under = lbl.find ( '_' , pos ) ;
            std :: string part = lbl.substr ( pos , under == std :: string :: npos ? std :: string :: npos : under - pos ) ;

            if ( !part.empty() ) {
                SDL_Surface * surf = TTF_RenderText_Blended ( font , part.c_str() , { 255,255,255,255 } ) ;
                if ( surf ) {
                    SDL_Texture * tex = SDL_CreateTextureFromSurface ( ren , surf ) ;
                    SDL_FreeSurface ( surf ) ;
                    if ( tex ) {
                        int tw = 0 , th = 0 ;
                        SDL_QueryTexture ( tex , nullptr , nullptr , &tw , &th ) ;
                        SDL_Rect dst { draw_x , sy + ( ( b.h - th ) / 2 ) , tw , th } ;
                        SDL_RenderCopy ( ren , tex , nullptr , &dst ) ;
                        SDL_DestroyTexture ( tex ) ;
                        draw_x += tw ;
                    }
                }
            }

            if ( under == std :: string :: npos ) break ;

            if ( input_idx < static_cast <int> ( b.inputs.size() ) ) {
                const block_input & inp = b.inputs[input_idx] ;
                SDL_Rect inp_r { draw_x , text_y , inp_w , inp_h } ;


                SDL_SetRenderDrawColor ( ren , 255 , 255 , 255 , inp.focused ? 255 : 210 ) ;
                SDL_RenderFillRect ( ren , &inp_r ) ;
                SDL_SetRenderDrawColor ( ren , inp.focused ? 0 : 80 , inp.focused ? 120 : 80 , inp.focused ? 255 : 80 , 255 ) ;
                SDL_RenderDrawRect ( ren , &inp_r ) ;

                fnt :: draw_text_centered ( ren , font , inp.value.c_str() , inp_r , { 0,0,0,255 } ) ;
                draw_x += inp_w + 2 ;
                ++input_idx ;
            }

            pos = under + 1 ;
        }

    }





    void block_workspace_render ( SDL_Renderer * ren ,
                                  block_workspace & ws ,
                                  SDL_Rect clip ,
                                  TTF_Font * font
                                  ) {
        if ( !ren ) {
            return ;
        }

        SDL_RenderSetClipRect ( ren , &clip ) ;

        for ( const auto & b : ws.blocks ) {

            if ( b.dragging ) {
                continue ;
            }

            const int sx = clip.x + b.x + ws.scroll_x ;
            const int sy = clip.y + b.y + ws.scroll_y ;


            if ( sx + b.w < clip.x || sx > clip.x + clip.w ) {
                continue ;
            }
            if ( sy + b.h < clip.y || sy > clip.y + clip.h ) {
                continue ;
            }

            draw_block ( ren , b , sx , sy , font ) ;
        }


        if ( ws.drag_idx >= 0 && ws.drag_idx < static_cast <int> ( ws.blocks.size() ) ) {
            const ui_block & b = ws.blocks[ws.drag_idx] ;
            const int sx = clip.x + b.x + ws.scroll_x ;
            const int sy = clip.y + b.y + ws.scroll_y ;
            draw_block ( ren , b , sx , sy , font ) ;
        }

        SDL_RenderSetClipRect ( ren , nullptr ) ;
    }



    int block_hit_test ( const block_workspace & ws ,
                         SDL_Rect clip ,
                         int mx , int my ) {

        for ( int i = static_cast <int> ( ws.blocks.size() ) - 1 ; i >= 0 ; --i ) {
            const ui_block & b = ws.blocks[i] ;
            const int sx = clip.x + b.x + ws.scroll_x ;
            const int sy = clip.y + b.y + ws.scroll_y ;
            if ( mx >= sx && mx < sx + b.w &&
                 my >= sy && my < sy + b.h ) {
                return i ;
            }
        }
        return -1 ;
    }



    void block_drag_begin ( block_workspace & ws , int idx , int mx , int my ) {
        if ( idx < 0 || idx >= static_cast <int> ( ws.blocks.size() ) ) {
            return ;
        }

        ws.drag_idx = idx ;
        ui_block & b = ws.blocks[idx] ;
        b.dragging = true ;
        b.drag_dx  = mx - b.x ;
        b.drag_dy  = my - b.y ;
        b.snap_to  = -1 ;

        ui_block copy = b ;
        ws.blocks.erase ( ws.blocks.begin() + idx ) ;
        ws.blocks.push_back ( copy ) ;
        ws.drag_idx = static_cast <int> ( ws.blocks.size() ) - 1 ;
    }

    void block_drag_update ( block_workspace & ws , int mx , int my ) {
        if ( ws.drag_idx < 0 || ws.drag_idx >= static_cast <int> ( ws.blocks.size() ) ) {
            return ;
        }

        ui_block & b = ws.blocks[ws.drag_idx] ;
        b.x = mx - b.drag_dx ;
        b.y = my - b.drag_dy ;
    }

    void block_drag_end ( block_workspace & ws ) {
        if ( ws.drag_idx < 0 || ws.drag_idx >= static_cast <int> ( ws.blocks.size() ) ) {
            return ;
        }
        ws.blocks[ws.drag_idx].dragging = false ;
        block_try_snap ( ws , ws.drag_idx ) ;
        ws.drag_idx = -1 ;
    }



    void block_try_snap ( block_workspace & ws , int idx ) {
        if ( idx < 0 || idx >= static_cast <int> ( ws.blocks.size() ) ) {
            return ;
        }
        ui_block & b = ws.blocks[idx] ;

        int best_dist = snap_dist + 1 ;
        int best_idx  = -1 ;

        for ( int i = 0 ; i < static_cast <int> ( ws.blocks.size() ) ; ++i ) {
            if ( i == idx ) {
                continue ;
            }
            const ui_block & other = ws.blocks[i] ;


            const int snap_x = other.x ;
            const int snap_y = other.y + other.h ;

            const int dx = std :: abs ( b.x - snap_x ) ;
            const int dy = std :: abs ( b.y - snap_y ) ;

            if ( dx < snap_dist && dy < snap_dist ) {
                const int dist = dx + dy ;
                if ( dist < best_dist ) {
                    best_dist = dist ;
                    best_idx  = i ;
                }
            }
        }

        if ( best_idx >= 0 ) {
            const ui_block & other = ws.blocks[best_idx] ;
            b.x = other.x ;
            b.y = other.y + other.h ;
            b.snap_to = ws.blocks[best_idx].id ;
        }
    }




    static constexpr int palette_item_h = 32 ;
    static constexpr int palette_pad = 4  ;

    static const struct { block_category cat ; const char * label ; } CATEGORIES[] = {
            { block_category :: motion , "Motion" } ,
            { block_category :: looks , "Looks" } ,
            { block_category :: events , "Events" } ,
            { block_category :: control , "Control" } ,
            { block_category :: operators , "Operators" } ,
            { block_category :: sound , "Sound" } ,
    } ;
    static constexpr int cat_count = 6 ;
    static constexpr int cat_w = 80 ;
    static constexpr int cat_item_h = 48 ;

    void block_palette_render ( SDL_Renderer * ren , SDL_Rect panel , TTF_Font * font , block_palette_state & state ) {
        if ( !ren ) {
            return ;
        }

        SDL_Rect sidebar { panel.x , panel.y , cat_w , panel.h } ;
        SDL_SetRenderDrawColor ( ren , 30 , 30 , 30 , 255 ) ;
        SDL_RenderFillRect ( ren , &sidebar ) ;

        for ( int i = 0 ; i < cat_count ; ++i ) {
            SDL_Rect r { sidebar.x , sidebar.y + i * cat_item_h , cat_w , cat_item_h } ;
            SDL_Color col = block_category_color ( CATEGORIES[i].cat ) ;

            if ( CATEGORIES[i].cat == state.selected_category ) {
                SDL_SetRenderDrawColor ( ren , col.r , col.g , col.b , 255 ) ;
            } else {
                SDL_SetRenderDrawColor ( ren , col.r/3 , col.g/3 , col.b/3 , 255 ) ;
            }
            SDL_RenderFillRect ( ren , &r ) ;
            SDL_SetRenderDrawColor ( ren , 0 , 0 , 0 , 255 ) ;
            SDL_RenderDrawRect ( ren , &r ) ;

            if ( font ) {
                fnt :: draw_text_centered ( ren , font , CATEGORIES[i].label , r , { 255,255,255,255 } ) ;
            }
        }


        SDL_Rect blocks_panel { panel.x + cat_w , panel.y , panel.w - cat_w , panel.h } ;
        SDL_RenderSetClipRect ( ren , &blocks_panel ) ;

        int y = blocks_panel.y + palette_pad ;
        for ( int i = 0 ; i < palette_count ; ++i ) {
            if ( PALETTE[i].cat != state.selected_category ) {
                continue ;
            }

            SDL_Rect r { blocks_panel.x + palette_pad , y , blocks_panel.w - ( palette_pad * 2 ) , palette_item_h } ;

            if ( r.y + r.h < blocks_panel.y || r.y > blocks_panel.y + blocks_panel.h ) {
                y += palette_item_h + palette_pad ;
                continue ;
            }

            SDL_Color col = block_category_color ( PALETTE[i].cat ) ;
            fill_rect ( ren , r , col ) ;
            draw_rect_outline ( ren , r , darker ( col , 40 ) ) ;

            if ( font ) {
                fnt :: draw_text_left ( ren , font , PALETTE[i].label , r , { 255,255,255,255 } , pad ) ;
            }

            y += palette_item_h + palette_pad ;
        }

        SDL_RenderSetClipRect ( ren , nullptr ) ;
    }

    bool block_palette_handle_click ( SDL_Rect panel , int mx , int my , block_palette_state & state ) {
        SDL_Rect sidebar { panel.x , panel.y , cat_w , panel.h } ;
        if ( mx >= sidebar.x && mx < sidebar.x + sidebar.w &&
             my >= sidebar.y && my < sidebar.y + sidebar.h ) {
            int i = ( my - sidebar.y ) / cat_item_h ;
            if ( i >= 0 && i < cat_count ) {
                state.selected_category = CATEGORIES[i].cat ;
                return true ;
            }
        }
        return false ;
    }

    bool block_palette_click ( SDL_Rect panel ,
                               int mx , int my ,
                               block_category & out_cat ,
                               std :: string & out_label ,
                               const block_palette_state & state
                               ) {

        const SDL_Rect blocks_panel { panel.x + cat_w , panel.y , panel.w - cat_w , panel.h } ;
        if ( mx < blocks_panel.x || mx > blocks_panel.x + blocks_panel.w ||
             my < blocks_panel.y || my > blocks_panel.y + blocks_panel.h ) {
            return false ;
        }

        int y = blocks_panel.y - palette_pad ;

        for ( int i = 0 ; i < palette_count ; ++i ) {
            if ( PALETTE[i].cat != state.selected_category ) {
                continue ;
            }

            SDL_Rect r { blocks_panel.x + palette_pad , y ,
                         blocks_panel.w - ( palette_pad * 2 ) , palette_item_h } ;

            if ( mx >= r.x && mx < r.x + r.w &&
                 my >= r.y && my < r.y + r.h
                 ) {
                out_cat = PALETTE[i].cat ;
                out_label = PALETTE[i].label ;
                return true ;
            }

            y += palette_item_h + palette_pad ;
        }

        return false ;

    }


    bool block_input_handle_click ( block_workspace & ws , SDL_Rect clip , int mx , int my ) {
        const int inp_h = 20 ;
        const int inp_w = 36 ;

        for ( int i = 0 ; i < static_cast <int> ( ws.blocks.size() ) ; ++i ) {
            ui_block & b = ws.blocks[i] ;
            if ( b.inputs.empty() ) {
                continue ;
            }

            const int sx = clip.x + b.x + ws.scroll_x ;
            const int sy = clip.y + b.y + ws.scroll_y ;
            const int text_y = sy + ( ( b.h - inp_h ) / 2 ) ;

            int draw_x = sx + pad ;
            int input_idx = 0 ;
            size_t pos = 0 ;
            std :: string lbl = b.label ;

            while ( pos <= lbl.size() && input_idx < static_cast <int> ( b.inputs.size() ) ) {
                size_t under = lbl.find ( '_' , pos ) ;
                std :: string part = lbl.substr ( pos , under == std :: string :: npos ? std :: string :: npos : under - pos ) ;
                draw_x += static_cast <int> ( part.size() ) * 7 ;

                if ( under == std :: string :: npos ) break ;

                SDL_Rect inp_r { draw_x , text_y , inp_w , inp_h } ;
                if ( mx >= inp_r.x && mx < inp_r.x + inp_r.w &&
                     my >= inp_r.y && my < inp_r.y + inp_r.h ) {

                    for ( auto & blk : ws.blocks ) {
                        for (auto &in: blk.inputs) {
                            in.focused = false;
                        }
                    }
                    b.inputs[input_idx].focused = true ;
                    ws.focused_block = i ;
                    ws.focused_input = input_idx ;
                    return true ;
                }

                draw_x += inp_w + 2 ;
                ++input_idx ;
                pos = under + 1 ;
            }
        }

        for ( auto & blk : ws.blocks ) {
            for (auto &in: blk.inputs) {
                in.focused = false;
            }
        }
        ws.focused_block = -1 ;
        ws.focused_input = -1 ;
        return false ;
    }

    void block_input_handle_key ( block_workspace & ws , SDL_Keycode key , const char * text ) {
        if ( ws.focused_block < 0 || ws.focused_block >= static_cast <int> ( ws.blocks.size() ) ) {
            return ;
        }

        if ( ws.focused_input < 0 ) {
            return ;
        }

        ui_block & b = ws.blocks[ws.focused_block] ;
        if ( ws.focused_input >= static_cast <int> ( b.inputs.size() ) ) {
            return ;
        }

        std :: string & val = b.inputs[ws.focused_input].value ;

        if ( key == SDLK_BACKSPACE ) {
            if ( !val.empty() ) val.pop_back() ;
        } else if ( key == SDLK_RETURN || key == SDLK_ESCAPE ) {
            b.inputs[ws.focused_input].focused = false ;
            ws.focused_block = -1 ;
            ws.focused_input = -1 ;
        } else if ( text && text[0] >= 32 ) {
            if ( val.size() < 8 ) val += text[0] ;
        }
    }

}
