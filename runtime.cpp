#include "runtime.h"
#include "sprite.h"
#include "texture_loader.h"
#include "pen.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <vector>
#include <string>
#include "layout.h"
#include "ui_menu.h"
#include "ui_button.h"
#include "backdrop.h"
#include "sound_manager.h"
#include "font_manager.h"
#include "ui_block.h"
#include "file_dialog.h"
#include "debug_logger.h"
#include "image_editor.h"
#include "interpreter.h"

namespace app {

    static bool init_sdl ( SDL_Window *& out_window ,
                           SDL_Renderer *& out_renderer ,
                           const runtimeConfig & cfg
                           ) {
        if ( SDL_Init ( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) != 0 ) {
            std :: fprintf ( stderr , "SDL_Init failed: %s\n" , SDL_GetError () ) ;
            return false ;
        }


        const int img_flags = IMG_INIT_PNG | IMG_INIT_JPG ;
        if ( ( IMG_Init ( img_flags ) & img_flags ) != img_flags ) {
            std :: fprintf ( stderr , "IMG_Init failed: %s\n" , IMG_GetError() ) ;
            SDL_Quit() ;
            return false ;
        }


        out_window = SDL_CreateWindow ( cfg . window_title . c_str() ,
                                        SDL_WINDOWPOS_CENTERED ,
                                        SDL_WINDOWPOS_CENTERED ,
                                        cfg . window_w , cfg . window_h ,
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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


    struct app_state {
        SDL_Window *window = nullptr ;
        SDL_Renderer * renderer = nullptr ;

        ui :: layout lay {} ;

        gfx :: sprite_manager sprite_mgr {} ;


        gfx :: pen_state pen {} ;
        gfx :: backdrop_manager backdrops {} ;
        snd :: sound_manager sounds {} ;
        gfx :: image_editor img_editor {} ;

        dbg :: debug_logger logger {} ;

        core :: interpreter interp {} ;
        bool sound_dragging = false ;

        fnt :: font_manager fonts {} ;

        ui :: menu menu_file {} ;
        ui :: menu menu_help {} ;
        ui :: menu menu_code {} ;
        ui :: menu menu_settings {} ;
        ui :: menu menu_run {} ;

        ui :: button btn_run {} ;
        ui :: button btn_stop {} ;

        bool running = true ;
        bool is_paused = false ;

        ui :: menu * open_menu = nullptr ;

        ui :: block_workspace workspace {} ;
    };

    static std :: vector < ui ::menu* > all_menus ( app_state & st ) {
        return { &st.menu_file , &st.menu_help ,
                 &st.menu_code , &st.menu_settings ,
                 &st.menu_run } ;
    }


    static void close_all_menus ( app_state & st ) {
        for ( auto * m : all_menus ( st ) ) {
            ui :: menu_close ( *m ) ;
        }
        st.open_menu = nullptr ;
    }



    static void build_menus ( app_state &st ) {
        st.menu_file.title = "File" ;
        st.menu_file.items = {
                { "New Project" , true , [&]{/* TODO: clear workspace */} } ,
                { "Save Project" , true , [&]{/* TODO: serialise */} } ,
                { "Load Project" , true , [&]{/* TODO: deserialise */} } ,
        };

        st.menu_help.title = "Help" ;
        st.menu_help.items = {
                { "Debug Logger" , true , [&]{
                    int ww , wh ;
                    SDL_GetWindowSize ( st.window , &ww , &wh ) ;
                    dbg :: logger_open ( st.logger , ww , wh ) ;
                    close_all_menus ( st ) ;
                } } ,
                { "Step-by-Step" , true , [&] { /* TODO: toggle debug mode */ } } ,
                { "About" , true , [&] { /* TODO: show about dialog */ } } ,
        } ;

        st.menu_code.title = "Code" ;
        st.menu_code.items = {
                { "Add Block " , true , [&] { /* TODO: show block picker */ } } ,
                { "Clear Script" , true , [&] { /* TODO: clear script */ } } ,
        } ;


        st.menu_settings.title = "Settings" ;
        st.menu_settings.items = {
                { "Background: Blue Sky" , true , [&] { gfx :: backdrop_set_active_by_name ( st.backdrops , "Blue Sky" ) ;
                    close_all_menus ( st ) ;  }  } ,
                { "Background: Green Field" , true , [&]{ gfx :: backdrop_set_active_by_name ( st.backdrops , "Green Field" ) ;
                    close_all_menus ( st ) ; } } ,
                { "Load Backdrop Image..." , true , [&] {
                    close_all_menus ( st ) ;
                    std :: string path = dlg :: open_image_dialog () ;
                    if ( !path.empty() ) {
                        int w = 0 , h = 0 ;
                        SDL_Texture * tex = gfx :: load_texture ( st.renderer , path , w , h ) ;
                        if ( tex ) {
                            gfx :: backdrop_add_texture ( st.backdrops , path , tex , w , h ) ;
                            gfx :: backdrop_set_active ( st.backdrops ,
                                 static_cast <int> ( st.backdrops.backdrops.size() ) -1 ) ;
                        }
                    }
                } } ,
                { "Add Sprite Image..." , true , [&] {
                    close_all_menus ( st ) ;
                    std :: string path = dlg :: open_image_dialog () ;
                    if ( !path.empty() ) {
                        int w = 0 , h = 0 ;
                        SDL_Texture * tex = gfx ::load_texture ( st.renderer , path , w , h ) ;
                        if ( tex ) {
                            gfx :: sprite s = gfx ::sprite_make (
                                    static_cast <int> ( st.sprite_mgr.sprites.size() ) , path.c_str() ) ;
                            s.draggable = true ;
                            gfx :: sprite_add_costume ( s , tex , w , h , "default" ) ;
                            gfx :: sprite_set_position ( s , 100.f , 100.0f ) ;
                            gfx :: sprite_manager_add ( st.sprite_mgr , s ) ;
                        }
                    }
                } } ,
                { "Add Sound..." , true , [&] {
                    close_all_menus ( st ) ;
                    std :: string path = dlg :: open_audio_dialog () ;
                    if ( !path.empty() ) {
                        snd ::sound_add ( st.sounds , path , path ) ;
                    }
                } } ,
        };

        st.menu_run.title = "Run" ;
        st.menu_run.items = {
                { "Run" , true , [&] {
                    core :: interpreter_run ( st.interp ) ;
                    close_all_menus ( st ) ;
                } } ,
                { "Pause" , true , [&] {
                    core :: interpreter_pause ( st.interp ) ;
                    close_all_menus ( st ) ;
                } } ,
                { "Stop" , true , [&] {
                    core :: interpreter_stop ( st.interp ) ;
                    close_all_menus ( st ) ;
                } } ,
        } ;
    }


    static void update_rects ( app_state &st ) {
        const SDL_Rect  & tb = st.lay.topBar ;

        st.menu_file.title_rect = ui :: topbar_menu_rect ( tb , 0 ) ;
        st.menu_help.title_rect = ui :: topbar_menu_rect ( tb , 1 ) ;
        st.menu_code.title_rect = ui :: topbar_menu_rect ( tb , 2 ) ;
        st.menu_settings.title_rect = ui :: topbar_menu_rect ( tb , 3 ) ;
        st.menu_run.title_rect = ui :: topbar_menu_rect ( tb , 4 ) ;


        st.btn_run.rect = ui :: topbar_right_rect ( tb , 1 , 60 , 26 ) ;
        st.btn_stop.rect = ui :: topbar_right_rect ( tb , 0 , 60 , 26 ) ;


        ui :: menu_layout ( st.menu_file ) ;
        ui :: menu_layout ( st.menu_help ) ;
        ui :: menu_layout ( st.menu_code ) ;
        ui :: menu_layout ( st.menu_settings ) ;
        ui :: menu_layout ( st.menu_run ) ;
    }





    static void handle_events ( app_state & st ) {
        SDL_Event e ;
        while ( SDL_PollEvent ( &e ) ) {

            if ( e.type == SDL_QUIT ) {
                st.running = false ;
                return ;
            }

            if ( e.type == SDL_KEYDOWN ) {
                if ( e.key.keysym.sym == SDLK_ESCAPE ) {
                    st.running = false ;
                    return ;
                }
            }

            if ( e.type == SDL_MOUSEBUTTONDOWN &&
                 e.button.button == SDL_BUTTON_LEFT ) {
                const int mx = e.button.x ;
                const int my = e.button.y ;

                if ( gfx :: editor_handle_click ( st.img_editor , mx , my ) ) {
                    return ;
                }
                if ( dbg :: logger_handle_click ( st.logger , mx , my ) ) {
                    return ;
                }

                if ( e.button.clicks == 2 &&
                     ui :: point_in_rect ( mx , my , st.lay.spriteBar ) ) {
                    if ( st.sprite_mgr.active >= 0 ) {
                        int ww , wh ;
                        SDL_GetWindowSize ( st.window , &ww , &wh ) ;
                        gfx::editor_open ( st.img_editor , st.renderer , ww , wh , st.sprite_mgr.active ) ;
                    }
                    return ;
                }

                bool menu_consumed = false ;


                for ( auto * m : all_menus ( st ) ) {
                    if ( ui :: menu_handle_click ( *m , mx , my ) ) {
                        for ( auto * other : all_menus ( st ) ) {
                            if ( other != m ) {
                                ui :: menu_close ( *other ) ;
                            }
                        }
                        st.open_menu = m -> open ? m : nullptr ;
                        menu_consumed = true ;
                        break ;
                    }
                }

                if ( !menu_consumed ) {
                    if ( ui :: button_handle_click ( st.btn_run , mx , my ) ||
                         ui :: button_handle_click ( st.btn_stop , mx , my ) ) {
                        menu_consumed = true ;
                    }
                }

                if ( !menu_consumed && st.open_menu ) {
                    close_all_menus ( st ) ;
                }

                if ( !menu_consumed &&
                     ui :: point_in_rect ( mx , my , st.lay.spriteBar ) ) {
                    if ( gfx :: sprite_manager_handle_click ( st.sprite_mgr , st.lay.spriteBar , mx , my ) ) {
                        menu_consumed = true ;
                    }

                }

                if ( !menu_consumed &&
                     ui :: point_in_rect ( mx , my , st.lay.stage ) ) {
                    if ( st.sprite_mgr.active >= 0 ) {
                        const gfx :: stage_rectangle sr {
                            st.lay.stage.x , st.lay.stage.y , st.lay.stage.w , st.lay.stage.h
                        } ;
                        gfx :: sprite_drag_begin ( st.sprite_mgr.sprites[st.sprite_mgr.active] ,
                                                   mx , my , sr ) ;
                    }
                    menu_consumed = true ;
                }

                if ( !menu_consumed ) {
                    if ( snd :: sound_handle_click ( st.sounds , st.lay.spriteBar , mx , my ) ) {
                        menu_consumed = true ;
                    }
                }

                if ( !menu_consumed &&
                     ui :: point_in_rect ( mx , my , st.lay.leftPanel ) ) {
                    ui :: block_category cat {} ;
                    std :: string label {} ;

                    if ( ui :: block_palette_click ( st.lay.leftPanel , mx , my , cat , label ) ) {
                        ui :: block_workspace_add ( st.workspace , label , cat ,
                                                    80 , 80 + static_cast <int> ( st.workspace.blocks.size() ) * 44
                                                    ) ;

                        menu_consumed = true ;
                    }
                }

                if ( !menu_consumed &&
                     ui :: point_in_rect ( mx , my , st.lay.workspace ) ) {
                    int idx = ui :: block_hit_test ( st.workspace , st.lay.workspace , mx , my ) ;
                    if ( idx >= 0 ) {
                        ui :: block_drag_begin ( st.workspace , idx , mx , my ) ;
                        menu_consumed = true ;
                    }
                }

            }

            if ( e.type == SDL_MOUSEMOTION ) {

                if ( st.sprite_mgr.active >= 0 ) {
                    const gfx :: stage_rectangle sr {
                        st.lay.stage.x , st.lay.stage.y , st.lay.stage.w , st.lay.stage.h
                    } ;
                    gfx :: sprite_drag_update ( st.sprite_mgr.sprites[st.sprite_mgr.active] ,
                                                e.motion.x , e.motion.y , sr ) ;
                }

                gfx :: editor_handle_drag ( st.img_editor , e.motion.x , e.motion.y ) ;
                snd :: sound_handle_drag ( st.sounds , st.lay.spriteBar ,
                                           e.motion.x , e.motion.y , st.sound_dragging ) ;

                ui :: block_drag_update ( st.workspace , e.motion.x , e.motion.y ) ;

            }

            if ( e.type == SDL_MOUSEBUTTONUP &&
                 e.button.button == SDL_BUTTON_LEFT ) {
                if ( st.sprite_mgr.active >= 0 ) {
                    gfx :: sprite_drag_end ( st.sprite_mgr.sprites[st.sprite_mgr.active] ) ;
                }

                st.sound_dragging = false ;

                gfx :: editor_handle_up ( st.img_editor , st.sprite_mgr , st.renderer ) ;
                ui :: block_drag_end ( st.workspace ) ;
            }

            if ( e.type == SDL_MOUSEWHEEL ) {
                dbg :: logger_handle_scroll ( st.logger , -e.wheel.y ) ;
            }
        }
    }



    namespace clr {
        constexpr SDL_Color menu_title { 45 , 45 , 45 , 255 } ;
        constexpr SDL_Color menu_border { 80 , 80 , 80 , 255 } ;
        constexpr SDL_Color panel_fill { 38 , 38 , 38 , 255 } ;
        constexpr SDL_Color panel_border { 70 , 70 , 70 , 255 } ;
        constexpr SDL_Color item_fill { 50 , 50 , 50 , 255 } ;
        constexpr SDL_Color item_border { 70 , 70 , 70 , 255 } ;
        constexpr SDL_Color item_dis { 35 , 35 , 35 , 255 } ;
        constexpr SDL_Color btn_run { 30 , 140 , 60 , 255 } ;
        constexpr SDL_Color btn_stop { 180 , 40 , 40 , 255 } ;
        constexpr SDL_Color btn_border { 100 , 100 , 100 , 255 } ;

    }


    static void render_frame ( app_state & st ) {
        SDL_SetRenderDrawColor ( st.renderer , 15 , 15 , 15 , 255 ) ;
        SDL_RenderClear ( st.renderer ) ;

        ui :: render_layout ( st.renderer , st.lay ) ;
        ui :: block_palette_render ( st.renderer , st.lay.leftPanel , st.fonts.medium ) ;
        ui :: block_workspace_render ( st.renderer , st.workspace , st.lay.workspace , st.fonts.medium ) ;
        gfx :: backdrop_render ( st.renderer , st.backdrops , st.lay.stage ) ;

        const gfx :: StageRect ps {
            st.lay.stage.x , st.lay.stage.y ,
            st.lay.stage.w , st.lay.stage.h
        };

        gfx :: pen_render ( st.renderer , st.pen , ps ) ;


        const gfx :: stage_rectangle sr {
            st.lay.stage.x , st.lay.stage.y ,
            st.lay.stage.w , st.lay.stage.h
        } ;

        for ( const auto & spr : st.sprite_mgr.sprites ) {
            gfx :: sprite_draw ( st.renderer , spr , sr ) ;
        }
        gfx :: sprite_manager_render ( st.renderer , st.sprite_mgr , st.lay.spriteBar , st.fonts.small ) ;

//        snd :: sound_render ( st.renderer , st.sounds , st.lay.spriteBar , st.fonts.small ) ;

        ui :: button_draw ( st.renderer , st.btn_run ,
                            st.fonts.medium , "Run" ,
                            clr :: btn_run , clr :: btn_border ) ;
        ui :: button_draw ( st.renderer , st.btn_stop ,
                            st.fonts.medium , "Stop" ,
                            clr :: btn_stop , clr :: btn_border ) ;



        for ( auto * m : all_menus ( st ) ) {
            ui :: menu_draw ( st.renderer , *m ,
                              st.fonts.medium ,
                              clr :: menu_title , clr :: menu_border ,
                              clr :: panel_fill , clr :: panel_border ,
                              clr :: item_fill , clr :: item_border ,
                              clr :: item_dis ) ;
        }

        dbg :: logger_render ( st.renderer , st.logger , st.fonts.small ) ;
        gfx :: editor_render ( st.renderer , st.img_editor , st.fonts.medium ) ;

        SDL_RenderPresent ( st.renderer ) ;

    }


    int run ( const runtimeConfig & cfg ) {
        app_state st {} ;

        if ( !init_sdl ( st.window , st.renderer , cfg ) ) {
            return 1 ;
        }

        gfx :: pen_init ( st.pen ) ;

        st.backdrops = gfx :: backdrop_make_defaults () ;

        snd :: sound_init() ;

        fnt :: font_init ( st.fonts , "assets/arial.ttf" ) ;



        build_menus ( st ) ;

        while ( st.running ) {
            int ww = 0 , wh = 0 ;
            SDL_GetWindowSize ( st.window , &ww , &wh ) ;
            st.lay = ui :: build_layout ( ww , wh ) ;

            update_rects ( st ) ;
            handle_events ( st ) ;
            render_frame ( st ) ;
        }

        snd ::sound_stop_all ( st.sounds) ;
        snd :: sound_quit () ;
        fnt :: font_quit ( st.fonts ) ;

        shutdown_sdl ( st.window , st.renderer ) ;
        return 0 ;
    }

}
