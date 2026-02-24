#include "image_editor.h"
#include "font_manager.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace gfx {



    static constexpr int ed_toolbar_h = 44 ;
    static constexpr int ed_pad = 8  ;
    static constexpr int ed_btn_w = 56 ;
    static constexpr int ed_btn_h = 32 ;


    static void ed_fill ( SDL_Renderer * ren , SDL_Rect r ,
                          Uint8 rr , Uint8 g , Uint8 b , Uint8 a = 255
                                  ) {
        SDL_SetRenderDrawBlendMode ( ren , a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE ) ;
        SDL_SetRenderDrawColor ( ren , rr , g , b , a ) ;
        SDL_RenderFillRect ( ren , &r ) ;
    }

    static void ed_outline ( SDL_Renderer * ren , SDL_Rect r ,
                             Uint8 rr , Uint8 g , Uint8 b
                             ) {
        SDL_SetRenderDrawColor ( ren , rr , g , b , 255 ) ;
        SDL_RenderDrawRect ( ren , &r ) ;
    }

    static bool ed_hit ( SDL_Rect r , int x , int y ) {
        return x >= r.x && x < r.x + r.w &&
               y >= r.y && y < r.y + r.h ;
    }


    static void canvas_draw_pixel ( image_editor & ed ,
                                    int cx , int cy ,
                                    SDL_Color col , int radius
                                    ) {
        if ( !ed.canvas ) {
            return ;
        }

        void * pixels = nullptr ;

        int pitch = 0 ;

        SDL_LockTexture ( ed.canvas , nullptr , &pixels , &pitch ) ;
        auto * px = static_cast <Uint32*> ( pixels ) ;

        for ( int dy = -radius ; dy <= radius ; ++dy ) {
            for ( int dx = -radius ; dx <= radius ; ++dx ) {
                if ( dx*dx + dy*dy > radius*radius ) {
                    continue ;
                }

                int nx = cx + dx ;
                int ny = cy + dy ;

                if ( nx < 0 || nx >= ed.canvas_w ) {
                    continue ;
                }
                if ( ny < 0 || ny >= ed.canvas_h ) {
                    continue ;
                }

                Uint32 c = ( ( Uint32 ) col.a << 24 ) | ( ( Uint32 ) col.r << 16 ) | ( ( Uint32 ) col.g <<  8 ) | ( Uint32 ) col.b ;
                px[ ( ny * ( pitch / 4 ) ) + nx ] = c ;
            }
        }

        SDL_UnlockTexture ( ed.canvas ) ;
    }


    static void canvas_draw_line ( image_editor & ed ,
                                   int x0 , int y0 ,
                                   int x1 , int y1 ,
                                   SDL_Color col , int radius
                                   ) {
        int dx = std :: abs ( x1 - x0 ) ;
        int dy = std :: abs ( y1 - y0 ) ;
        int sx = x0 < x1 ? 1 : -1 ;
        int sy = y0 < y1 ? 1 : -1 ;
        int err = dx - dy ;

        while ( true ) {
            canvas_draw_pixel ( ed , x0 , y0 , col , radius ) ;
            if ( x0 == x1 && y0 == y1 ) {
                break ;
            }

            int e2 = 2 * err ;

            if ( e2 > -dy ) {
                err -= dy ; x0 += sx ;
            }
            if ( e2 <  dx ) {
                err += dx ; y0 += sy ;
            }

        }
    }


    static void canvas_draw_circle ( image_editor & ed ,
                                     int cx , int cy , int r ,
                                     SDL_Color col , int thickness
                                     ) {
        int x = 0 , y = r , d = 3 - ( 2 * r ) ;
        while ( y >= x ) {
            canvas_draw_pixel ( ed , cx + x , cy + y , col , thickness ) ;
            canvas_draw_pixel ( ed , cx - x , cy + y , col , thickness ) ;
            canvas_draw_pixel ( ed , cx + x , cy - y , col , thickness ) ;
            canvas_draw_pixel ( ed , cx - x , cy - y , col , thickness ) ;
            canvas_draw_pixel ( ed , cx + y , cy + x , col , thickness ) ;
            canvas_draw_pixel ( ed , cx - y , cy + x , col , thickness ) ;
            canvas_draw_pixel ( ed , cx + y , cy - x , col , thickness ) ;
            canvas_draw_pixel ( ed , cx - y , cy - x , col , thickness ) ;
            if ( d < 0 ) {
                d += ( x * 4 ) + 6 ;
            } else {
                d += ( ( x - y ) * 4 ) + 10 ;
                --y ;
            }
            ++x ;
        }
    }


    static void canvas_draw_rect_outline ( image_editor & ed ,
                                           int x0 , int y0 ,
                                           int x1 , int y1 ,
                                           SDL_Color col , int thickness
                                           ) {
        canvas_draw_line ( ed , x0 , y0 , x1 , y0 , col , thickness ) ;
        canvas_draw_line ( ed , x1 , y0 , x1 , y1 , col , thickness ) ;
        canvas_draw_line ( ed , x1 , y1 , x0 , y1 , col , thickness ) ;
        canvas_draw_line ( ed , x0 , y1 , x0 , y0 , col , thickness ) ;
    }


    static void canvas_fill ( image_editor & ed ,
                              int sx , int sy ,
                              SDL_Color fill_col
                              ) {
        if ( !ed.canvas ) {
            return ;
        }

        void * pixels = nullptr ;

        int pitch = 0 ;

        SDL_LockTexture ( ed.canvas , nullptr , &pixels , &pitch ) ;

        auto * px = static_cast <Uint32*> ( pixels ) ;

        const int stride = pitch / 4 ;

        if ( sx < 0 || sx >= ed.canvas_w ||
             sy < 0 || sy >= ed.canvas_h ) {
            SDL_UnlockTexture ( ed.canvas ) ;
            return ;
        }

        Uint32 target = px[ ( sy * stride ) + sx ] ;
        Uint32 replacement = ( ( Uint32 ) fill_col.a << 24 ) |
                             ( ( Uint32 ) fill_col.r << 16 ) |
                             ( ( Uint32 ) fill_col.g <<  8 ) |
                             ( Uint32 ) fill_col.b ;

        if ( target == replacement ) {
            SDL_UnlockTexture ( ed.canvas ) ;
            return ;
        }

        std :: vector < std :: pair < int , int > > q ;
        q.push_back ( { sx , sy } ) ;

        while ( !q.empty() ) {
            int x = q.back().first ;
            int y = q.back().second ;
            q.pop_back() ;

            if ( x < 0 || x >= ed.canvas_w ) {
                continue ;
            }
            if ( y < 0 || y >= ed.canvas_h ) {
                continue ;
            }
            if ( px[ y * stride + x ] != target ) {
                continue ;
            }

            px[ ( y * stride ) + x ] = replacement ;
            q.push_back ( { x + 1 , y } ) ;
            q.push_back ( { x - 1 , y } ) ;
            q.push_back ( { x , y + 1 } ) ;
            q.push_back ( { x , y - 1 } ) ;
        }

        SDL_UnlockTexture ( ed.canvas ) ;
    }


    static bool screen_to_canvas ( const image_editor & ed ,
                                   int mx , int my ,
                                   int & out_cx , int & out_cy
                                   ) {
        if ( !ed_hit ( ed.canvas_rect , mx , my ) ) {
            return false ;
        }
        out_cx = ( mx - ed.canvas_rect.x ) * ed.canvas_w / ed.canvas_rect.w ;
        out_cy = ( my - ed.canvas_rect.y ) * ed.canvas_h / ed.canvas_rect.h ;
        return true ;
    }


    static void editor_build_rects ( image_editor & ed ) {
        const SDL_Rect & w = ed.window_rect ;

        ed.toolbar = { w.x , w.y , w.w , ed_toolbar_h } ;

        int bx = w.x + ed_pad ;
        const int by = w.y + ( ed_toolbar_h - ed_btn_h ) / 2 ;

        ed.btn_pen = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_eraser = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_line = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_circle = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_rect_tool = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_fill = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ( ed_pad * 3 ) ;
        ed.btn_erase_all = { bx , by , ed_btn_w , ed_btn_h } ; bx += ed_btn_w + ed_pad ;
        ed.btn_size_down = { bx , by , 24 , ed_btn_h } ; bx += 24 + 4 ;
        ed.btn_size_up   = { bx , by , 24 , ed_btn_h } ; bx += 24 + ed_pad ;

        ed.color_picker.rect = { bx , by , ed_btn_h , ed_btn_h } ;

        ed.btn_apply = { w.x + w.w - ed_pad - ed_btn_w ,
                         by , ed_btn_w , ed_btn_h } ;
        ed.btn_close = { w.x + w.w - ( ed_pad * 2 ) - ( ed_btn_w * 2 ) ,
                         by , ed_btn_w , ed_btn_h } ;

        const int cy = w.y + ed_toolbar_h + ed_pad ;
        const int ch = w.h - ed_toolbar_h - ( ed_pad * 2 ) ;
        const int cw = w.w - ( ed_pad * 2 ) ;
        ed.canvas_rect = { w.x + ed_pad , cy , cw , ch } ;
    }


    void editor_open ( image_editor & ed ,
                       SDL_Renderer  * ren ,
                       int win_w , int win_h , int sprite_idx
                       ) {
        if ( ed.open ) {
            return ;
        }

        const int ew = std :: min ( 700 , win_w - 40 ) ;
        const int eh = std :: min ( 600 , win_h - 40 ) ;

        ed.window_rect = {
                ( win_w - ew ) / 2 ,
                ( win_h - eh ) / 2 ,
                ew , eh
        } ;

        editor_build_rects ( ed ) ;

        ed.canvas = SDL_CreateTexture ( ren ,
                                        SDL_PIXELFORMAT_ARGB8888 ,
                                        SDL_TEXTUREACCESS_STREAMING ,
                                        ed.canvas_w , ed.canvas_h ) ;
        if ( ed.canvas ) {
            void * pixels = nullptr ;

            int pitch = 0 ;

            SDL_LockTexture ( ed.canvas , nullptr , &pixels , &pitch ) ;

            std :: memset ( pixels , 255 , static_cast <size_t> ( pitch * ed.canvas_h ) ) ;
            SDL_UnlockTexture ( ed.canvas ) ;
        }

        ed.target_sprite = sprite_idx ;
        ed.tool = editor_tool :: pen ;
        ed.color = { 0 , 0 , 0 , 255 } ;
        ed.brush_size = 0.75 ;
        ed.drawing = false ;
        ed.open = true ;
    }


    void editor_close ( image_editor & ed ) {
        if ( ed.canvas ) {
            SDL_DestroyTexture ( ed.canvas ) ;
            ed.canvas = nullptr ;
        }
        ed.open = false ;
        ed.target_sprite = -1 ;
    }


    void editor_render ( SDL_Renderer * ren , image_editor & ed , TTF_Font * font) {

        if ( !ed.open || !ren ) {
            return ;
        }

        SDL_SetRenderDrawBlendMode ( ren , SDL_BLENDMODE_BLEND ) ;
        SDL_SetRenderDrawColor ( ren , 0 , 0 , 0 , 160 ) ;
        SDL_Rect full { 0 , 0 , 4000 , 4000 } ;
        SDL_RenderFillRect ( ren , &full ) ;

        ed_fill    ( ren , ed.window_rect , 35 , 35 , 35 ) ;
        ed_outline ( ren , ed.window_rect , 80 , 80 , 80 ) ;

        ed_fill    ( ren , ed.toolbar , 45 , 45 , 45 ) ;
        ed_outline ( ren , ed.toolbar , 70 , 70 , 70 ) ;


        struct ToolBtn {
            SDL_Rect *r ;
            const char * lbl ;
            editor_tool t ;
        } btns[] = {
                { &ed.btn_pen , "Pen" , editor_tool :: pen } ,
                { &ed.btn_eraser , "Erase" , editor_tool :: eraser } ,
                { &ed.btn_line , "Line" , editor_tool :: line } ,
                { &ed.btn_circle , "Circle" , editor_tool :: circle } ,
                { &ed.btn_rect_tool , "Rect" , editor_tool :: rect } ,
                { &ed.btn_fill , "Fill" , editor_tool :: fill } ,
        } ;

        for ( auto & b : btns ) {
            const bool active = ( ed.tool == b.t ) ;
            if ( active ) {
                ed_fill ( ren , *b.r , 60 , 120 , 200 ) ;
            } else {
                ed_fill ( ren , *b.r , 55 , 55  , 55  ) ;
            }
            ed_outline ( ren , *b.r , 90 , 90 , 90 ) ;
            if ( font ) {
                fnt :: draw_text_centered ( ren , font , b.lbl , *b.r , { 220,220,220,255 } ) ;
            }
        }

        ed_fill ( ren , ed.btn_erase_all , 180 , 60 , 60 ) ;
        ed_outline ( ren , ed.btn_erase_all , 90  , 90 , 90 ) ;
        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Clear" , ed.btn_erase_all , { 255,255,255,255 } ) ;
        }

        const int by = ed.toolbar.y + ( ed_toolbar_h - ed_btn_h ) / 2 ;
        ed_fill    ( ren , ed.btn_size_down , 55 , 55 , 55 ) ;
        ed_outline ( ren , ed.btn_size_down , 90 , 90 , 90 ) ;
        if ( font ) fnt :: draw_text_centered ( ren , font , "-" , ed.btn_size_down , { 220,220,220,255 } ) ;

        ed_fill    ( ren , ed.btn_size_up , 55 , 55 , 55 ) ;
        ed_outline ( ren , ed.btn_size_up , 90 , 90 , 90 ) ;
        if ( font ) fnt :: draw_text_centered ( ren , font , "+" , ed.btn_size_up , { 220,220,220,255 } ) ;

// Show current size
        char sz_buf[8] ;
        snprintf ( sz_buf , sizeof(sz_buf) , "%d" , ed.brush_size ) ;
        SDL_Rect sz_lbl { ed.btn_size_down.x - 24 , by , 22 , ed_btn_h } ;
        if ( font ) fnt :: draw_text_centered ( ren , font , sz_buf , sz_lbl , { 180,180,180,255 } ) ;


        ed_fill ( ren , ed.color_picker.rect , ed.color.r , ed.color.g , ed.color.b ) ;
        ed_outline ( ren , ed.color_picker.rect , 200 , 200 , 200 ) ;


        ed_fill ( ren , ed.btn_apply , 40  , 140 , 60 ) ;
        ed_outline ( ren , ed.btn_apply , 80  , 80  , 80 ) ;
        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Apply" , ed.btn_apply , { 255,255,255,255 } ) ;
        }


        ed_fill ( ren , ed.btn_close , 160 , 40  , 40 ) ;
        ed_outline ( ren , ed.btn_close , 80  , 80  , 80 ) ;
        if ( font ) {
            fnt :: draw_text_centered ( ren , font , "Close" , ed.btn_close , { 255,255,255,255 } ) ;
        }

        if ( ed.canvas ) {
            SDL_RenderCopy ( ren , ed.canvas , nullptr , &ed.canvas_rect ) ;
        }
        ed_outline ( ren , ed.canvas_rect , 100 , 100 , 100 ) ;
        if ( ed.drawing ) {
            const float sx = (float)ed.canvas_rect.w / ed.canvas_w ;
            const float sy = (float)ed.canvas_rect.h / ed.canvas_h ;

            auto to_screen_x = [&]( int cx ) { return ed.canvas_rect.x + (int)(cx * sx) ; } ;
            auto to_screen_y = [&]( int cy ) { return ed.canvas_rect.y + (int)(cy * sy) ; } ;

            SDL_SetRenderDrawBlendMode ( ren , SDL_BLENDMODE_BLEND ) ;
            SDL_SetRenderDrawColor ( ren , ed.color.r , ed.color.g , ed.color.b , 180 ) ;

            if ( ed.tool == editor_tool :: line ) {
                SDL_RenderDrawLine ( ren ,
                                     to_screen_x ( ed.start_x ) , to_screen_y ( ed.start_y ) ,
                                     to_screen_x ( ed.last_x  ) , to_screen_y ( ed.last_y  ) ) ;

            } else if ( ed.tool == editor_tool :: rect ) {
                int rx = to_screen_x ( std :: min ( ed.start_x , ed.last_x ) ) ;
                int ry = to_screen_y ( std :: min ( ed.start_y , ed.last_y ) ) ;
                int rw = (int)( std :: abs ( ed.last_x - ed.start_x ) * sx ) ;
                int rh = (int)( std :: abs ( ed.last_y - ed.start_y ) * sy ) ;
                SDL_Rect pr { rx , ry , rw , rh } ;
                SDL_RenderDrawRect ( ren , &pr ) ;

            } else if ( ed.tool == editor_tool :: circle ) {
                int r  = std :: max ( std :: abs ( ed.last_x - ed.start_x ) ,
                                      std :: abs ( ed.last_y - ed.start_y ) ) ;
                int cx = to_screen_x ( ed.start_x ) ;
                int cy = to_screen_y ( ed.start_y ) ;
                int rs = (int)( r * sx ) ;
                for ( int deg = 0 ; deg < 360 ; deg += 2 ) {
                    float a1 = deg       * 3.14159f / 180.0f ;
                    float a2 = (deg + 2) * 3.14159f / 180.0f ;
                    SDL_RenderDrawLine ( ren ,
                                         cx + (int)( rs * cosf(a1) ) , cy + (int)( rs * sinf(a1) ) ,
                                         cx + (int)( rs * cosf(a2) ) , cy + (int)( rs * sinf(a2) ) ) ;
                }
            }
        }
    }


    bool editor_handle_click ( image_editor & ed , int mx , int my ) {
        if ( !ed.open ) {
            return false ;
        }
        if ( !ed_hit ( ed.window_rect , mx , my ) ) {
            return false ;
        }

        if ( ed_hit ( ed.btn_pen , mx , my ) ) {
            ed.tool = editor_tool :: pen ;
            return true ;
        }
        if ( ed_hit ( ed.btn_eraser , mx , my ) ) {
            ed.tool = editor_tool :: eraser ;
            return true ;
        }
        if ( ed_hit ( ed.btn_line , mx , my ) ) {
            ed.tool = editor_tool :: line ;
            return true ;
        }
        if ( ed_hit ( ed.btn_circle , mx , my ) ) {
            ed.tool = editor_tool :: circle ;
            return true ;
        }
        if ( ed_hit ( ed.btn_rect_tool , mx , my ) ) {
            ed.tool = editor_tool :: rect ;
            return true ;
        }
        if ( ed_hit ( ed.btn_fill , mx , my ) ) {
            ed.tool = editor_tool::fill ;
            return true ;
        }
        if ( ed_hit ( ed.btn_erase_all , mx , my ) ) {
            if ( ed.canvas ) {
                void * pixels = nullptr ;
                int pitch = 0 ;
                SDL_LockTexture ( ed.canvas , nullptr , &pixels , &pitch ) ;
                auto * px = static_cast<Uint32*> ( pixels ) ;
                for ( int i = 0 ; i < ed.canvas_w * ed.canvas_h ; ++i ) {
                    px[i] = 0xFFFFFFFF ;
                }
                SDL_UnlockTexture ( ed.canvas ) ;
            }
            return true ;
        }


        if ( ed_hit ( ed.color_picker.rect , mx , my ) ) {
            static const SDL_Color colors[] = {
                    { 0   , 0   , 0   , 255 } ,
                    { 255 , 255 , 255 , 255 } ,
                    { 255 , 0   , 0   , 255 } ,
                    { 0   , 200 , 0   , 255 } ,
                    { 0   , 0   , 255 , 255 } ,
                    { 255 , 200 , 0   , 255 } ,
                    { 255 , 100 , 0   , 255 } ,
                    { 150 , 0   , 200 , 255 } ,
            } ;
            static int ci = 0 ;
            ci = ( ci + 1 ) % 8 ;
            ed.color = colors[ci] ;
            return true ;
        }

        if ( ed_hit ( ed.btn_close , mx , my ) ) {
            editor_close ( ed ) ;
            return true ;
        }


        if ( ed_hit ( ed.btn_apply , mx , my ) ) {
            return true ;
        }


        if ( ed_hit ( ed.btn_size_down , mx , my ) ) {
            if ( ed.brush_size > 1 ) ed.brush_size-- ;
            return true ;
        }
        if ( ed_hit ( ed.btn_size_up , mx , my ) ) {
            if ( ed.brush_size < 20 ) ed.brush_size++ ;
            return true ;
        }

        int cx = 0 , cy = 0 ;
        if ( screen_to_canvas ( ed , mx , my , cx , cy ) ) {
            ed.drawing = true ;
            ed.start_x = cx ;  ed.start_y = cy ;
            ed.last_x  = cx ;  ed.last_y  = cy ;

            if ( ed.tool == editor_tool :: pen ) {
                canvas_draw_pixel ( ed , cx , cy , ed.color , ed.brush_size ) ;
            } else if ( ed.tool == editor_tool :: eraser ) {
                canvas_draw_pixel ( ed , cx , cy , { 255,255,255,255 } , ed.brush_size * 2 ) ;
            } else if ( ed.tool == editor_tool :: fill ) {
                canvas_fill ( ed , cx , cy , ed.color ) ;
                ed.drawing = false ;
            }
        }

        return true ;
    }


    void editor_handle_drag ( image_editor & ed , int mx , int my ) {
        if ( !ed.open || !ed.drawing ) {
            return ;
        }

        int cx = 0 , cy = 0 ;
        if ( !screen_to_canvas ( ed , mx , my , cx , cy ) ) {
            return ;
        }

        if ( ed.tool == editor_tool :: pen ) {
            canvas_draw_line ( ed , ed.last_x , ed.last_y , cx , cy , ed.color , ed.brush_size ) ;
            ed.last_x = cx ; ed.last_y = cy ;
        } else if ( ed.tool == editor_tool :: eraser ) {
            canvas_draw_line ( ed , ed.last_x , ed.last_y ,
                               cx , cy ,
                               { 255,255,255,255 } , ed.brush_size * 2 ) ;
            ed.last_x = cx ; ed.last_y = cy ;
        } else {
            ed.last_x = cx ; ed.last_y = cy ;
        }
    }


    void editor_handle_up ( image_editor  & ed , sprite_manager & sm , SDL_Renderer   * ren ) {
        if ( !ed.open ) {
            return ;
        }


        if ( ed.drawing ) {
            if ( ed.tool == editor_tool :: line ) {
                canvas_draw_line ( ed ,
                                   ed.start_x , ed.start_y ,
                                   ed.last_x  , ed.last_y  ,
                                   ed.color , ed.brush_size
                                   ) ;
            } else if ( ed.tool == editor_tool :: circle ) {
                int rx = std :: abs ( ed.last_x - ed.start_x ) ;
                int ry = std :: abs ( ed.last_y - ed.start_y ) ;
                int r = std :: max ( rx , ry ) ;
                canvas_draw_circle ( ed , ed.start_x , ed.start_y , r ,
                                     ed.color , ed.brush_size ) ;
            } else if ( ed.tool == editor_tool :: rect ) {
                canvas_draw_rect_outline ( ed , ed.start_x , ed.start_y ,
                                           ed.last_x  , ed.last_y  ,
                                           ed.color , ed.brush_size
                                           ) ;
            }

            ed.drawing = false ;
        }


        if ( ed.target_sprite >= 0 &&
             ed.target_sprite < static_cast <int> ( sm.sprites.size() ) &&
             ed.canvas && ren ) {

            SDL_Texture * target = SDL_CreateTexture (
                    ren ,
                    SDL_PIXELFORMAT_ARGB8888 ,
                    SDL_TEXTUREACCESS_TARGET ,
                    ed.canvas_w , ed.canvas_h ) ;

            if ( target ) {
                SDL_SetRenderTarget ( ren , target ) ;
                SDL_RenderCopy ( ren , ed.canvas , nullptr , nullptr ) ;
                SDL_SetRenderTarget ( ren , nullptr ) ;

                sprite & s = sm.sprites[ed.target_sprite] ;
                if ( s.costumes.empty() ) {
                    sprite_add_costume ( s , target , ed.canvas_w , ed.canvas_h , "drawn" ) ;
                } else {
                    sprite_replace_current_texture ( s , target , ed.canvas_w , ed.canvas_h ) ;
                }
            }
        }
    }

}