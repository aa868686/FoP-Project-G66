#include "runtime.h"
#include "sprite.h"
#include "texture_loader.h"
#include "pen.h"
#include <SDL2/SDL_image.h>
#include <cstdio>

namespace app {

    static bool init_sdl ( SDL_Window *& out_window ,
                           SDL_Renderer *& out_renderer ,
                           const runtimeConfig & cfg
                           ) {
        if ( SDL_Init ( SDL_INIT_VIDEO ) != 0 ) {
            std :: fprintf ( stderr , "SDL_Init failed: %s\n" , SDL_GetError () ) ;
            return false ;
        }


        // SDL_image init
        const int img_flags = IMG_INIT_PNG | IMG_INIT_JPG ;
        const int initted = IMG_Init ( img_flags ) ;
        if ( ( initted & img_flags ) != img_flags ) {
            std :: fprintf ( stderr , "IMG_Init failed: %s\n" , IMG_GetError() ) ;
            SDL_Quit() ;
            return false ;
        }


        out_window = SDL_CreateWindow ( cfg . window_title . c_str() ,
                                        SDL_WINDOWPOS_CENTERED ,
                                        SDL_WINDOWPOS_CENTERED ,
                                        cfg . window_w , cfg . window_h ,
                                        SDL_WINDOW_SHOWN
                                        ) ;

        if ( !out_window ) {
            std :: fprintf ( stderr , "SDL_CreatWindow failed: %s\n" , SDL_GetError() ) ;
            IMG_Quit() ;
            SDL_Quit() ;
            return false ;
        }

        out_renderer = SDL_CreateRenderer ( out_window , -1 ,
                                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
                                            ) ;

        if ( !out_renderer ) {
            std :: fprintf ( stderr , "SDL_CreateRenderer failed: %s\n" , SDL_GetError() ) ;
            SDL_DestroyWindow ( out_window ) ;
            IMG_Quit() ;
            SDL_Quit() ;
            return false ;
        }


        // Enable linear filtering
        SDL_SetHint ( SDL_HINT_RENDER_SCALE_QUALITY , "1" ) ;

        return true ;

    }


    static void shutdown_sdl ( SDL_Window * window , SDL_Renderer *renderer ) {
        if ( renderer ) {
            SDL_DestroyRenderer ( renderer ) ;
        }
        if ( window ) {
            SDL_DestroyWindow ( window ) ;
        }
        IMG_Quit() ;
        SDL_Quit() ;
    }


    // Simple color cycle for pen (black -> red -> green -> blue -> black)
    static SDL_Color pen_next_color ( SDL_Color c ) {
        if ( c.r == 0 && c.g == 0 && c.b == 0 ) {
            return SDL_Color { 255 , 0 , 0 , 255 } ;
        }
        if ( c.r == 255 && c.g == 0 && c.b == 0 ) {
            return SDL_Color { 0 , 255 , 0 , 255 } ;
        }
        if ( c.r == 0 && c.g == 255 && c.b == 0 ) {
            return SDL_Color { 0 , 0 , 255 , 255 } ;
        }

        return SDL_Color { 0 , 0 , 0 , 255 } ;
    }


    static SDL_Texture * sprite_current_texture ( const gfx :: sprite & spr ) {
        if ( spr . costumes . empty() ) {
            return nullptr ;
        }
        if ( spr . current_costume < 0 || spr . current_costume >= (int) spr.costumes.size() ) {
            return nullptr ;
        }

        return spr . costumes[spr.current_costume] . texture ;
    }


    int run_basic_sprite_demo ( const runtimeConfig & cfg ,
                                const std :: string & sprite_image_path
                                ) {
        SDL_Window *window = nullptr ;
        SDL_Renderer  *renderer = nullptr ;

        if ( !init_sdl ( window , renderer , cfg ) ) {
            return 1 ;
        }


        // Define stage as full window
        gfx :: stage_rectangle stage{} ;
        stage.x = 0 ;
        stage.y = 0 ;
        stage.w = cfg.window_w ;
        stage.h = cfg.window_h ;


        // Create one sprite
        gfx :: sprite spr = gfx :: sprite_make ( 1 , "Sprite1") ;
        gfx :: sprite_set_draggable ( spr , true ) ;


        // Start at center of stage
        gfx :: sprite_set_position ( spr , stage.w * 0.5f , stage.h * 0.5f ) ;


        // Load texture via SDL_image
        int tex_w = 0 , tex_h = 0 ;
        SDL_Texture * tex = gfx :: load_texture ( renderer , sprite_image_path , tex_w , tex_h ) ;
        if ( !tex ) {
            shutdown_sdl ( window , renderer ) ;
            return 2 ;
        }



        // Add as costume
        gfx :: sprite_add_costume ( spr , tex , tex_w , tex_h , "default" ) ;


        // Pen state (persistent)
        gfx :: pen_state pen {} ;
        gfx :: pen_init ( pen ) ;

        bool running = true ;




        // Simple keyboard controls : W/S = move, A/D = turn
        const float move_step = 5.0f ;
        const float turn_step = 5.0f ;

        while ( running ) {
            // Track previous sprite position so we can add pen points when it moves.
            const float prev_x = spr.x ;
            const float prev_y = spr.y ;


            SDL_Event e ;
            while ( SDL_PollEvent ( &e ) ) {
                if ( e.type == SDL_QUIT ) {
                    running = false ;
                } else if ( e.type == SDL_MOUSEBUTTONDOWN ) {
                    if ( e.button.button == SDL_BUTTON_LEFT ) {
                        gfx :: sprite_drag_begin ( spr , e.button.x , e.button.y , stage ) ;
                    }
                } else if ( e.type == SDL_MOUSEMOTION ) {
                    gfx :: sprite_drag_update ( spr , e.motion.x , e.motion.y , stage ) ;
                } else if ( e.type == SDL_MOUSEBUTTONUP ) {
                    if ( e.button.button == SDL_BUTTON_LEFT ) {
                        gfx :: sprite_drag_end ( spr ) ;
                    }
                } else if ( e.type == SDL_KEYDOWN ) {
                    // allow exit with ESC
                    if ( e.key.keysym.sym == SDLK_ESCAPE ) {
                        running = false ;
                        continue ;
                    }


                    // Pen controls
                    if ( e.key.keysym.sym == SDLK_p ) {
                        // Toggle pen down/up using the sprite center
                        if ( !pen.is_down ) {
                            gfx :: pen_down ( pen , spr.x , spr.y ) ;
                        } else {
                            gfx :: pen_up ( pen ) ;
                        }
                    } else if ( e.key.keysym.sym == SDLK_c ) {
                        gfx :: pen_set_color ( pen , pen_next_color ( pen.color ) ) ;
                    } else if ( e.key.keysym.sym == SDLK_e ) {
                        gfx :: pen_erase_all ( pen ) ;
                    } else if ( e.key.keysym.sym == SDLK_LEFTBRACKET ) {
                        gfx :: pen_change_size ( pen , -1 ) ;
                    } else if ( e.key.keysym.sym == SDLK_RIGHTBRACKET ) {
                        gfx :: pen_change_size ( pen , +1 ) ;
                    } else if ( e.key.keysym.sym == SDLK_t ) {
                        // Stamp current sprite costume with same render params as sprite_draw.
                        float w = 0.0f , h = 0.0f ;
                        if ( gfx :: sprite_get_render_size ( spr , w , h ) ) {
                            SDL_FRect dst ;
                            dst.x = ( spr.x - ( w * 0.5f ) ) + ( float ) stage.x ;
                            dst.y = ( spr.x - ( h * 0.5f ) ) + ( float ) stage.y ;
                            dst.w = w ;
                            dst.h = h ;

                            SDL_FPoint center { dst.w * 0.5f , dst.h * 0.5f } ;
                            const auto angle = (double)(spr.direction_deg - 90.0f ) ;

                            gfx :: pen_stamp ( pen , sprite_current_texture ( spr ) , dst , angle , center ) ;
                        }
                    }
                }
            }


            // Continuous keyboard state ( smooth movement )
            const Uint8 *ks = SDL_GetKeyboardState ( nullptr ) ;
            if ( ks[SDL_SCANCODE_W] ) {
                gfx :: sprite_move_steps ( spr , move_step ) ;
            }
            if ( ks[SDL_SCANCODE_S] ) {
                gfx :: sprite_move_steps ( spr , -move_step ) ;
            }
            if ( ks[SDL_SCANCODE_A] ) {
                gfx :: sprite_turn_degree ( spr , -turn_step ) ;
            }
            if ( ks[SDL_SCANCODE_D] ) {
                gfx :: sprite_turn_degree ( spr , turn_step ) ;
            }


            // Keep sprite inside stage
            gfx :: sprite_clamp_to_stage ( spr , stage ) ;


            // If pen is down and sprite moved (including dragging), add a point.
            if ( pen.is_down && ( spr.x != prev_x || spr.y != prev_y ) ) {
                gfx :: pen_add_point ( pen , spr.x , spr.y ) ;
            }

            // Render pipeline: background -> pen -> sprite
            SDL_SetRenderDrawColor ( renderer , 25 , 25 , 25 , 255 ) ;
            SDL_RenderClear ( renderer ) ;

            gfx :: StageRect pen_stage { stage.x , stage.y , stage.w , stage.h } ;
            gfx :: pen_render ( renderer , pen , pen_stage ) ;

            gfx :: sprite_draw ( renderer , spr , stage ) ;

            SDL_RenderPresent ( renderer ) ;

        }



        // Cleanup texture created by load_texture
        SDL_DestroyTexture ( tex ) ;

        shutdown_sdl ( window , renderer ) ;
        return 0 ;

    }

}
