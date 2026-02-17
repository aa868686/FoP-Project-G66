#include "runtime.h"
#include "sprite.h"
#include "texture_loader.h"
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

        bool running = true ;

        // Simple keyboard controls : W/S = move, A/D = turn
        const float move_step = 5.0f ;
        const float turn_step = 5.0f ;

        while ( running ) {
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

            // Render
            SDL_SetRenderDrawColor ( renderer , 25 , 25 , 25 , 255 ) ;
            SDL_RenderClear ( renderer ) ;

            gfx :: sprite_draw ( renderer , spr , stage ) ;

            SDL_RenderPresent ( renderer ) ;

        }



        // Cleanup texture created by load_texture
        SDL_DestroyTexture ( tex ) ;

        shutdown_sdl ( window , renderer ) ;
        return 0 ;

    }

}
