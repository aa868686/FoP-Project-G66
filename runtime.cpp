#include "runtime.h"
#include "sprite.h"
#include "texture_loader.h"
#include "pen.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
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
#include "block_compiler.h"
#include "block.h"
#include "project_serializer.h"
#include "variable.h"
#include "my_block.h"

namespace app {

    static bool init_sdl(SDL_Window *&out_window,
                         SDL_Renderer *&out_renderer,
                         const runtimeConfig &cfg
    ) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return false;
        }


        const int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
        if ((IMG_Init(img_flags) & img_flags) != img_flags) {
            std::fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
            SDL_Quit();
            return false;
        }


        out_window = SDL_CreateWindow(cfg.window_title.c_str(),
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      cfg.window_w, cfg.window_h,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if (!out_window) {
            std::fprintf(stderr, "SDL_CreatWindow failed: %s\n", SDL_GetError());
            IMG_Quit();
            SDL_Quit();
            return false;
        }

        out_renderer = SDL_CreateRenderer(out_window, -1,
                                          SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!out_renderer) {
            std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(out_window);
            IMG_Quit();
            SDL_Quit();
            return false;
        }


        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        return true;
    }


    static void shutdown_sdl(SDL_Window *window, SDL_Renderer *renderer) {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        IMG_Quit();
        SDL_Quit();
    }


    struct app_state {
        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;
        int interp_log_forwarded = 0 ;

        struct sprite_info_input {
            SDL_Rect rect {} ;
            std :: string value {} ;
            bool focused = false ;
            enum field_type { field_x , field_y , field_size , field_direction } ;
            field_type target = field_x ;
        };

        ui::layout lay{};

        ui :: block_palette_state palette_state {} ;

        ui::button btn_step {} ;

        gfx::sprite_manager sprite_mgr{};

        core :: variable_store variables {} ;


        gfx::pen_state pen{};
        gfx::backdrop_manager backdrops{};
        snd::sound_manager sounds{};
        gfx::image_editor img_editor{};

        dbg::debug_logger logger{};


        core::interpreter interp{};
        bool sound_dragging = false;

        fnt::font_manager fonts{};


        ui::menu menu_file{};
        ui::menu menu_help{};
        ui::menu menu_code{};
        ui::menu menu_settings{};
        ui :: menu menu_background {} ;

        ui::button btn_run{};
        ui::button btn_stop{};
        ui :: button btn_pause {} ;

        myblock :: func_store func_store {} ;
        myblock :: my_block_editor my_block_ed {} ;

        bool running = true;
        bool is_paused = false;

        bool sprite_clicked = false ;
        bool sprite_dragged = false ;

        bool step_mode = false ;
        std :: vector < core :: Block * > compiled_blocks {} ;




        bool context_menu_open = false ;
        int context_menu_block_idx = -1 ;
        SDL_Rect context_menu_rect {} ;

        ui::menu *open_menu = nullptr;


        std :: vector < sprite_info_input > info_inputs {} ;

        ui::block_workspace workspace{};
    };

    static std::vector<ui::menu *> all_menus(app_state &st) {
        return {&st.menu_file, &st.menu_help,
                &st.menu_code, &st.menu_settings ,
                &st.menu_background
        };
    }


    static void close_all_menus(app_state &st) {
        for (auto *m: all_menus(st)) {
            ui::menu_close(*m);
        }
        st.open_menu = nullptr;
    }


    static void build_menus ( app_state &st ) {
        st.menu_file.title = "File";
        st.menu_file.items = {
                {"New Project",  true, [&] {
                    core :: interpreter_stop ( st.interp ) ;
                    compiler :: free_compiled ( st.compiled_blocks ) ;

                    st.workspace.blocks.clear() ;
                    st.workspace.next_id = 0 ;
                    st.workspace.scroll_x = 0 ;
                    st.workspace.scroll_y = 0 ;
                    st.workspace.drag_idx = -1 ;
                    st.workspace.focused_block = -1 ;
                    st.workspace.focused_input = -1 ;

                    if ( st.sprite_mgr.active >= 0 ) {
                        gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                        s.x = st.lay.stage.w * 0.5f ;
                        s.y = st.lay.stage.h * 0.5f ;
                        s.size_percent = 100.0f ;
                        s.direction_deg = 90.0f ;
                        s.say_visible = true ;
                    }

                    snd :: sound_stop_all ( st.sounds ) ;

                    st.info_inputs.clear () ;

                    st.step_mode = false ;
                    st.is_paused = false ;

                    dbg :: logger_log ( st.logger , "New Project Created." ) ;
                    close_all_menus ( st ) ;


                }},
                {"Save Project", true, [&] {
                    std :: string path = serial :: save_dialog () ;
                    if ( !path.empty() ) {
                        auto res = serial :: project_save (
                                path ,
                                st.sprite_mgr ,
                                st.backdrops ,
                                st.workspace ,
                                st.variables ,
                                st.sounds
                        ) ;
                        if ( res.ok ) {
                            dbg :: logger_log ( st.logger , "Project saved." ) ;
                        } else {
                            dbg :: logger_log ( st.logger , ("Save failed: " + res.error).c_str() , dbg :: log_level :: error ) ;
                        }
                    }
                    close_all_menus ( st ) ;
                }},
                {"Load Project", true, [&] {
                    std :: string path = serial :: load_dialog () ;
                    if ( !path.empty() ) {
                        core :: interpreter_stop ( st.interp ) ;
                        compiler :: free_compiled ( st.compiled_blocks ) ;
                        auto res = serial :: project_load (
                                path ,
                                st.sprite_mgr ,
                                st.backdrops ,
                                st.workspace ,
                                st.variables ,
                                st.sounds ,
                                st.renderer
                        ) ;
                        if ( res.ok ) {
                            st.info_inputs.clear () ;
                            st.step_mode = false ;
                            st.is_paused = false ;
                            dbg :: logger_log ( st.logger , "Project loaded." ) ;
                        } else {
                            dbg :: logger_log ( st.logger , ("Load failed: " + res.error ).c_str() , dbg :: log_level :: error ) ;
                        }
                    }
                    close_all_menus ( st ) ;
                }},
        };

        st.menu_help.title = "Help";
        st.menu_help.items = {
                {"Debug Logger", true, [&] {
                    int ww, wh;
                    SDL_GetWindowSize(st.window, &ww, &wh);
                    dbg::logger_open(st.logger, ww, wh);
                    close_all_menus(st);
                }},
                {"Step-by-Step", true, [&] {
                    st.step_mode  = !st.step_mode ;
                    dbg :: logger_log ( st.logger , st.step_mode ? "Step mode ON" : "Step mode OFF" ) ;
                    close_all_menus ( st ) ;
                }},
                {"About",        true, [&] { /* TODO: show about dialog */ }},
        };

        st.menu_code.title = "Code";
        st.menu_code.items = {
                {"Add Block ",   true, [&] { /* TODO: show block picker */ }},
                {"Clear Script", true, [&] {
                    st.workspace.blocks.clear() ;
                    st.workspace.next_id = 0 ;
                    st.workspace.drag_idx = -1 ;
                    st.workspace.focused_block = -1 ;
                    st.workspace.focused_input = -1 ;
                    close_all_menus ( st ) ;
                }},
        };


        st.menu_settings.title = "Settings";
        st.menu_settings.items = {
                {"Background: Blue Sky",    true, [&] {
                    gfx::backdrop_set_active_by_name(st.backdrops, "Blue Sky");
                    close_all_menus(st);
                }},
                {"Background: Green Field", true, [&] {
                    gfx::backdrop_set_active_by_name(st.backdrops, "Green Field");
                    close_all_menus(st);
                }},
                {"Load Backdrop Image...",  true, [&] {
                    close_all_menus(st);
                    std::string path = dlg::open_image_dialog();
                    if (!path.empty()) {
                        int w = 0, h = 0;
                        SDL_Texture *tex = gfx::load_texture(st.renderer, path, w, h);
                        if (tex) {
                            gfx::backdrop_add_texture(st.backdrops, path, tex, w, h);
                            gfx::backdrop_set_active(st.backdrops,
                                                     static_cast <int> ( st.backdrops.backdrops.size()) - 1);
                        }
                    }
                }},
                {"Add Sprite Image...",     true, [&] {
                    close_all_menus(st);
                    std::string path = dlg::open_image_dialog();
                    if (!path.empty()) {
                        int w = 0, h = 0;
                        SDL_Texture *tex = gfx::load_texture(st.renderer, path, w, h);
                        if (tex) {
                            gfx::sprite s = gfx::sprite_make(
                                    static_cast <int> ( st.sprite_mgr.sprites.size()), path.c_str());
                            s.draggable = true;
                            gfx::sprite_add_costume(s, tex, w, h, path.c_str() , path.c_str() );
                            gfx::sprite_set_position(s, 100.f, 100.0f);
                            gfx::sprite_manager_add(st.sprite_mgr, s);
                        }
                    }
                }},
                {"Add Sound...",            true, [&] {
                    close_all_menus(st);
                    std::string path = dlg::open_audio_dialog();
                    if (!path.empty()) {
                        snd::sound_add(st.sounds, path, path);
                    }
                }},

        };

        st.menu_background.title = "Background" ;
        st.menu_background.items = {
                { "backdrop1" , true , [&] {
                    gfx :: backdrop_set_active ( st.backdrops , 2 ) ;
                    close_all_menus ( st ) ;
                }} ,
                { "backdrop2" , true , [&] {
                    gfx :: backdrop_set_active ( st.backdrops , 3 ) ;
                    close_all_menus ( st ) ;
                }} ,
                { "backdrop3" , true , [&] {
                    gfx :: backdrop_set_active ( st.backdrops , 4 ) ;
                    close_all_menus ( st ) ;
                }} ,
        } ;


    }


    static void update_rects(app_state &st) {
        const SDL_Rect &tb = st.lay.topBar;

        st.menu_file.title_rect = ui::topbar_menu_rect(tb, 0);
        st.menu_help.title_rect = ui::topbar_menu_rect(tb, 1);
        st.menu_code.title_rect = ui::topbar_menu_rect(tb, 2);
        st.menu_settings.title_rect = ui::topbar_menu_rect(tb, 3);
        st.menu_background.title_rect = ui :: topbar_menu_rect ( tb , 4 ) ;


        st.btn_stop.rect = ui :: topbar_right_rect (tb, 0, 60, 26);
        st.btn_pause.rect = ui :: topbar_right_rect ( tb , 1 , 60 , 26 ) ;
        st.btn_run.rect = ui :: topbar_right_rect (tb, 2, 60, 26);
        st.btn_step.rect = ui :: topbar_right_rect ( tb , 3 , 100 , 26 ) ;

        ui::menu_layout(st.menu_file, st.fonts.medium);
        ui::menu_layout(st.menu_help, st.fonts.medium);
        ui::menu_layout(st.menu_code, st.fonts.medium);
        ui::menu_layout(st.menu_settings, st.fonts.medium);
        ui :: menu_layout ( st.menu_background , st.fonts.medium ) ;


        if ( st.sprite_mgr.active >= 0 ) {
            gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
            SDL_Rect p = st.lay.spriteInfo ;

            struct field_def {
                app_state :: sprite_info_input :: field_type f ;
                float val ;
                int lx , ly ;
            };

            field_def fields[4] = {
                    { app_state :: sprite_info_input :: field_x , s.x , 20 , 8 } ,
                    { app_state :: sprite_info_input :: field_y , s.y , 140 , 8 } ,
                    { app_state :: sprite_info_input :: field_size , s.size_percent , 20 ,  38 } ,
                    { app_state :: sprite_info_input :: field_direction , s.direction_deg , 140 , 38 } ,
            };

            if ( st.info_inputs.size() != 4 ) {
                st.info_inputs.clear() ;
                for ( auto & fd : fields ) {
                    app_state :: sprite_info_input inp ;
                    inp.target = fd.f ;
                    inp.rect = { p.x + fd.lx + 32 , p.y + fd.ly - 2 , 52 , 20 } ;
                    char buf[16] ;
                    snprintf ( buf , sizeof ( buf ) , "%.0f" , fd.val ) ;
                    inp.value = buf ;
                    st.info_inputs.push_back ( inp ) ;
                }
            } else {
                for ( int i = 0 ; i < 4 ; ++i ) {
                    st.info_inputs[i].rect = { p.x + fields[i].lx + 32 , p.y + fields[i].ly - 2 , 52 , 20 } ;
                    if ( !st.info_inputs[i].focused ) {
                        char buf[16] ;
                        snprintf ( buf , sizeof ( buf ) , "%.0f" , fields[i].val ) ;
                        st.info_inputs[i].value = buf ;
                    }
                }
            }

        } else {
            st.info_inputs.clear() ;
        }


    }


    static void handle_events(app_state &st) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {

            if (e.type == SDL_QUIT) {
                st.running = false;
                return;
            }

            if ( e.type == SDL_KEYDOWN ) {
                if ( e.key.keysym.sym == SDLK_BACKSPACE ||
                     e.key.keysym.sym == SDLK_RETURN ||
                     e.key.keysym.sym == SDLK_ESCAPE ||
                     e.key.keysym.sym == SDLK_LEFT ||
                     e.key.keysym.sym == SDLK_RIGHT ||
                     e.key.keysym.sym == SDLK_DELETE
                        ) {
                    ui :: block_input_handle_key ( st.workspace , e.key.keysym.sym , nullptr ) ;
                }
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    bool any_focused = false ;
                    for ( auto & inp : st.info_inputs ) {
                        if ( inp.focused ) {
                            any_focused = true ;
                            break ;
                        }
                    }
                    if ( !any_focused ) {
                        st.running = false ;
                        return ;
                    }
                }
            }

            if ( e.type == SDL_KEYDOWN ) {
                if ( e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER ) {
                    bool any_focused = false ;
                    for ( auto & inp : st.info_inputs ) {
                        if ( inp.focused ) { any_focused = true ; break ; }
                    }
                    if ( st.workspace.focused_input < 0 && !any_focused ) {
                        st.btn_run.on_click() ;
                    }
                }
            }


            if ( e.type == SDL_TEXTINPUT ) {
                ui :: block_input_handle_key ( st.workspace , SDLK_UNKNOWN , e.text.text ) ;
                myblock :: editor_handle_text ( st.my_block_ed , e.text.text ) ;
                for ( auto & inp: st.info_inputs ) {
                    if ( inp.focused && inp.value.size() < 6 ) {
                        inp.value += e.text.text ;
                    }
                }
            }

            if ( e.type == SDL_KEYDOWN ) {
                if ( e.type == SDL_KEYDOWN ) {
                    if ( e.key.keysym.sym == SDLK_BACKSPACE ||
                         e.key.keysym.sym == SDLK_RETURN ||
                         e.key.keysym.sym == SDLK_ESCAPE
                            ) {
                        ui :: block_input_handle_key ( st.workspace , e.key.keysym.sym , nullptr ) ;
                    }

                    for ( auto & inp: st.info_inputs ) {
                        if ( !inp.focused ) {
                            continue ;
                        }
                        if ( e.key.keysym.sym == SDLK_BACKSPACE ) {
                            if ( !inp.value.empty() ) inp.value.pop_back() ;
                        }
                        if ( e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_ESCAPE ) {
                            if ( st.sprite_mgr.active >= 0 && !inp.value.empty() ) {
                                gfx::sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                                try {
                                    float val = std :: stof ( inp.value ) ;
                                    switch ( inp.target ) {
                                        case app_state :: sprite_info_input :: field_x : s.x = val ;
                                            break ;
                                        case app_state :: sprite_info_input :: field_y : s.y = val ;
                                            break ;
                                        case app_state :: sprite_info_input :: field_size : gfx :: sprite_set_size ( s , val ) ;
                                            break ;
                                        case app_state :: sprite_info_input :: field_direction : gfx :: sprite_set_direction ( s , val ) ;
                                            break ;
                                    }
                                } catch (...) {}
                            }

                            inp.focused = false ;
                            SDL_StopTextInput() ;
                        }
                    }
                }
                if ( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE ) {
                    myblock :: editor_handle_backspace ( st.my_block_ed ) ;
                }
            }


            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT) {

                const int mx = e.button.x;
                const int my = e.button.y;

                if ( st.context_menu_open ) {
                    if ( ui :: point_in_rect ( mx , my , st.context_menu_rect ) ) {
                        if ( st.context_menu_block_idx >= 0 &&
                             st.context_menu_block_idx < (int)st.workspace.blocks.size()
                                ) {
                            ui :: block_workspace_remove ( st.workspace , st.workspace.blocks[st.context_menu_block_idx].id ) ;
                        }
                    }
                    st.context_menu_open = false ;
                    st.context_menu_block_idx = -1 ;
                    return ;
                }

                if (gfx :: editor_handle_click (st.img_editor, mx, my )) {
                    return;
                }

                {
                    int out_new = -1 ;
                    if ( myblock :: editor_handle_click ( st.my_block_ed , st.func_store , mx , my , out_new ) ) {
                        if ( out_new >= 0 ) {
                            const auto & f = st.func_store.funcs[out_new] ;
                            std :: string def_lbl = myblock :: func_define_label ( f ) ;
                            ui :: block_workspace_add ( st.workspace , def_lbl , ui :: block_category :: my_blocks , 50 , 50 ) ;
                            std :: string call_lbl = myblock :: func_call_label ( f ) ;
                            st.palette_state.my_block_labels.push_back ( call_lbl ) ;
                        }
                        return ;
                    }
                }


                for ( auto & inp : st.info_inputs ) {
                    if ( inp.focused && st.sprite_mgr.active >= 0 && !inp.value.empty() ) {
                        gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active ] ;
                        try {
                            float val = std :: stof ( inp.value ) ;
                            switch ( inp.target ) {
                                case app_state :: sprite_info_input :: field_x : s.x = val ;
                                    break ;
                                case app_state :: sprite_info_input :: field_y : s.y = val ;
                                    break ;
                                case app_state :: sprite_info_input :: field_size : gfx :: sprite_set_size ( s , val ) ;
                                    break ;
                                case app_state :: sprite_info_input :: field_direction : gfx :: sprite_set_direction ( s , val ) ;
                                    break ;
                            }
                        } catch (...) {}
                    }
                    for ( auto & inp : st.info_inputs ) {
                        inp.focused = false;
                    }
                }


                for ( auto & inp : st.info_inputs ) {
                    if ( ui :: point_in_rect ( mx , my , inp.rect ) ) {
                        inp.focused = true ;
                        SDL_StartTextInput () ;
                        break ;
                    }
                }

                if ( st.sprite_mgr.active >= 0 ) {
                    SDL_Rect panel = st.lay.spriteInfo ;
                    gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                    SDL_Rect show_btn { panel.x + panel.w - 120 , panel.y + 8 , 50 , 20 } ;
                    SDL_Rect hide_btn { panel.x + panel.w - 64  , panel.y + 8 , 50 , 20 } ;

                    if ( ui :: point_in_rect ( mx , my , show_btn ) ) {
                        gfx :: sprite_set_visible ( s , true ) ;
                    } else if ( ui :: point_in_rect ( mx , my , hide_btn ) ) {
                        gfx :: sprite_set_visible ( s , false ) ;
                    }
                }

                if ( dbg :: logger_handle_click(st.logger, mx, my)) {
                    return;
                }

                if (e.button.clicks == 2 &&
                    ui::point_in_rect(mx, my, st.lay.spriteBar)) {
                    if (st.sprite_mgr.active >= 0) {
                        int ww, wh;
                        SDL_GetWindowSize(st.window, &ww, &wh);
                        gfx::editor_open(st.img_editor, st.renderer, ww, wh, st.sprite_mgr.active);
                    }
                    return;
                }

                bool menu_consumed = false;


                for (auto *m: all_menus(st)) {
                    if (ui::menu_handle_click(*m, mx, my)) {
                        for (auto *other: all_menus(st)) {
                            if (other != m) {
                                ui::menu_close(*other);
                            }
                        }
                        st.open_menu = m->open ? m : nullptr;
                        menu_consumed = true;
                        break;
                    }
                }

                if (!menu_consumed) {
                    if (ui::button_handle_click(st.btn_run, mx, my) ||
                        ui::button_handle_click(st.btn_stop, mx, my)) {
                        menu_consumed = true;
                    }
                }

                if ( ui :: button_handle_click ( st.btn_pause , mx , my ) ) {
                    menu_consumed = true ;
                }

                if ( st.step_mode ) {
                    if ( ui ::button_handle_click ( st.btn_step , mx , my ) ) {
                        menu_consumed = true ;
                    }
                }

                if (!menu_consumed && st.open_menu) {
                    close_all_menus(st);
                }

                if (!menu_consumed &&
                    ui::point_in_rect(mx, my, st.lay.spriteBar)) {
                    if (gfx::sprite_manager_handle_click(st.sprite_mgr, st.lay.spriteBar, mx, my)) {
                        menu_consumed = true;
                    }

                }

                if (!menu_consumed &&
                    ui::point_in_rect(mx, my, st.lay.stage)) {
                    if (st.sprite_mgr.active >= 0) {
                        const gfx::stage_rectangle sr{
                                st.lay.stage.x, st.lay.stage.y, st.lay.stage.w, st.lay.stage.h
                        } ;
                        gfx::sprite_drag_begin(st.sprite_mgr.sprites[st.sprite_mgr.active],
                                               mx, my, sr);
                        st.sprite_clicked = true ;
                    }
                    menu_consumed = true;
                }

                if (!menu_consumed) {
                    if (snd::sound_handle_click(st.sounds, st.lay.spriteBar, mx, my)) {
                        menu_consumed = true;
                    }
                }

                if (!menu_consumed &&
                    ui::point_in_rect (mx, my, st.lay.leftPanel) ) {

                    if ( ui :: block_palette_handle_click ( st.lay.leftPanel , mx , my , st.palette_state ) ) {
                        menu_consumed = true ;
                    } else {
                        ui :: block_category cat {} ;
                        std :: string label {} ;

                        if ( ui :: block_palette_click ( st.lay.leftPanel , mx , my , cat , label , st.palette_state ) ) {
                            if ( label == "Make a Block" ) {
                                int ww , wh ;
                                SDL_GetWindowSize (SDL_GetWindowFromID ( 1 ) , &ww , &wh ) ;
                                myblock :: editor_open ( st.my_block_ed , ww , wh ) ;
                            } else {
                                ui :: block_workspace_add ( st.workspace , label , cat , 80 ,
                                                            80 + static_cast <int> ( st.workspace.blocks.size() ) * 44 ) ;
                            }
                            menu_consumed = true ;
                        }
                    }
                }

                if (!menu_consumed &&
                    ui::point_in_rect(mx, my, st.lay.workspace)) {

                    if ( ui :: block_input_handle_click ( st.workspace , st.lay.workspace , mx , my ) ) {
                        menu_consumed = true ;
                    } else {
                        int idx = ui::block_hit_test(st.workspace, st.lay.workspace, mx, my);
                        if (idx >= 0) {
                            ui::block_drag_begin(st.workspace, idx, mx, my);
                            menu_consumed = true;
                        }
                    }
                }

            }

            if ( e.type == SDL_MOUSEBUTTONDOWN &&
                 e.button.button == SDL_BUTTON_RIGHT ) {
                const int mx = e.button.x ;
                const int my = e.button.y ;
                if ( ui :: point_in_rect ( mx , my , st.lay.workspace ) ) {
                    int idx = ui :: block_hit_test ( st.workspace , st.lay.workspace , mx , my ) ;
                    if ( idx >= 0 ) {
                        st.context_menu_open = true ;
                        st.context_menu_block_idx = idx ;
                        st.context_menu_rect = { mx , my , 100 , 28 } ;
                    } else {
                        st.context_menu_open = false ;
                    }
                }
            }

            if (e.type == SDL_MOUSEMOTION) {

                if (st.sprite_mgr.active >= 0) {
                    const gfx::stage_rectangle sr{
                            st.lay.stage.x, st.lay.stage.y, st.lay.stage.w, st.lay.stage.h
                    };
                    gfx::sprite_drag_update(st.sprite_mgr.sprites[st.sprite_mgr.active],
                                            e.motion.x, e.motion.y, sr);
                    if ( st.sprite_clicked ) {
                        st.sprite_dragged = true ;
                    }
                }

                gfx::editor_handle_drag(st.img_editor, e.motion.x, e.motion.y);
                snd::sound_handle_drag(st.sounds, st.lay.spriteBar,
                                       e.motion.x, e.motion.y, st.sound_dragging);

                ui::block_drag_update(st.workspace, e.motion.x, e.motion.y);

                for ( auto * m : all_menus ( st ) ) {
                    m -> title_hovered = ui :: point_in_rect ( e.motion.x , e.motion.y , m -> title_rect ) ;
                    if ( !m -> open ) {
                        continue ;
                    }

                    for ( auto & item : m -> items ) {
                        item.hovered = ui :: point_in_rect ( e.motion.x , e.motion.y , item.rect ) ;
                    }
                }
            }


            if (e.type == SDL_MOUSEBUTTONUP &&
                e.button.button == SDL_BUTTON_LEFT) {
                if (st.sprite_mgr.active >= 0) {
                    gfx::sprite_drag_end(st.sprite_mgr.sprites[st.sprite_mgr.active]);
                }
                if ( st.sprite_clicked && !st.sprite_dragged ) {
                    if ( !st.sounds.sounds.empty() ) {
                        snd :: sound_play ( st.sounds.sounds[0] ) ;
                    }
                }

                st.sprite_clicked = false ;
                st.sprite_dragged = false ;

                st.sound_dragging = false;

                gfx::editor_handle_up(st.img_editor, st.sprite_mgr, st.renderer);
                ui::block_drag_end(st.workspace);
            }

            if (e.type == SDL_MOUSEWHEEL) {
                dbg::logger_handle_scroll(st.logger, -e.wheel.y);
            }
        }
    }



    namespace clr {
        constexpr SDL_Color menu_title{45, 45, 45, 255};
        constexpr SDL_Color menu_border{80, 80, 80, 255};
        constexpr SDL_Color panel_fill{38, 38, 38, 255};
        constexpr SDL_Color panel_border{70, 70, 70, 255};
        constexpr SDL_Color item_fill{50, 50, 50, 255};
        constexpr SDL_Color item_border{70, 70, 70, 255};
        constexpr SDL_Color item_dis{35, 35, 35, 255};
        constexpr SDL_Color btn_run{30, 140, 60, 255};
        constexpr SDL_Color btn_stop{180, 40, 40, 255};
        constexpr SDL_Color btn_border{100, 100, 100, 255};

    }


    static void render_frame(app_state &st) {
        SDL_SetRenderDrawColor(st.renderer, 15, 15, 15, 255);
        SDL_RenderClear(st.renderer);

        ui::render_layout(st.renderer, st.lay);
        ui::block_palette_render( st.renderer , st.lay.leftPanel , st.fonts.medium , st.palette_state , st.workspace ) ;
        ui::block_workspace_render(st.renderer, st.workspace, st.lay.workspace, st.fonts.medium);
        Uint32 now = SDL_GetTicks();
        st.interp.results.erase(
                std::remove_if(st.interp.results.begin(), st.interp.results.end(),
                               [now](const core::result_display& r) { return now > r.show_until; }),
                st.interp.results.end());

        for (const auto& rd : st.interp.results) {
            if (rd.block_line >= 0 && rd.block_line < (int)st.workspace.blocks.size()) {
                const auto& b = st.workspace.blocks[rd.block_line];
                int bx = st.lay.workspace.x + b.x + st.workspace.scroll_x;
                int by = st.lay.workspace.y + b.y + st.workspace.scroll_y;

                int rw = (int)rd.text.size() * 8 + 16;
                int rh = 22;
                SDL_Rect rr { bx + b.w + 6, by + (b.h - rh)/2, rw, rh };

                SDL_SetRenderDrawColor(st.renderer, 30, 30, 30, 220);
                SDL_RenderFillRect(st.renderer, &rr);
                SDL_SetRenderDrawColor(st.renderer, 100, 200, 100, 255);
                SDL_RenderDrawRect(st.renderer, &rr);
                fnt::draw_text_left(st.renderer, st.fonts.small, rd.text.c_str(), rr, {100, 255, 100, 255});
            }
        }
        gfx::backdrop_render(st.renderer, st.backdrops, st.lay.stage);

        const gfx::StageRect ps{
                st.lay.stage.x, st.lay.stage.y,
                st.lay.stage.w, st.lay.stage.h
        };

        gfx::pen_render(st.renderer, st.pen, ps);


        const gfx::stage_rectangle sr{
                st.lay.stage.x, st.lay.stage.y,
                st.lay.stage.w, st.lay.stage.h
        };

        SDL_RenderSetClipRect ( st.renderer , &st.lay.stage ) ;
        for ( const auto & spr : st.sprite_mgr.sprites ) {
            gfx::sprite_draw(st.renderer, spr, sr, st.fonts.medium);
        }
        int var_y = st.lay.stage.y + 8;
        int var_x = st.lay.stage.x + 8;
        for (const auto& pair : st.interp.store.variables) {
            if (!pair.second.visible) continue;
            std::string display = pair.first + " = " + value_to_string(pair.second.value);
            SDL_Rect var_rect { var_x, var_y, 160, 22 };
            SDL_SetRenderDrawColor(st.renderer, 20, 20, 20, 180);
            SDL_RenderFillRect(st.renderer, &var_rect);
            SDL_SetRenderDrawColor(st.renderer, 200, 200, 50, 255);
            SDL_RenderDrawRect(st.renderer, &var_rect);
            fnt::draw_text_left(st.renderer, st.fonts.small, display.c_str(), var_rect, {255, 220, 50, 255});
            var_y += 26;
        }
        SDL_RenderSetClipRect ( st.renderer , nullptr ) ;

        gfx::sprite_manager_render(st.renderer, st.sprite_mgr, st.lay.spriteBar, st.fonts.small);

//        snd :: sound_render ( st.renderer , st.sounds , st.lay.spriteBar , st.fonts.small ) ;

        ui :: button_draw ( st.renderer , st.btn_run ,
                            st.fonts.medium , "Run" ,
                            clr :: btn_run , clr :: btn_border
        ) ;
        ui :: button_draw ( st.renderer , st.btn_stop ,
                            st.fonts.medium , "Stop" ,
                            clr :: btn_stop , clr :: btn_border
        ) ;

        SDL_Color pause_col = st.interp.is_paused ? SDL_Color { 60 , 180 , 60 , 255 } : SDL_Color { 200 , 160 , 0 , 255 } ;

        const char * pause_lbl = st.interp.is_paused ? "Resume" : "Pause" ;
        ui :: button_draw ( st.renderer , st.btn_pause ,
                            st.fonts.medium , pause_lbl ,
                            pause_col , clr :: btn_border
        ) ;

        if ( st.step_mode ) {
            ui :: button_draw ( st.renderer , st.btn_step ,
                                st.fonts.medium , "Step-by-Step" ,
                                SDL_Color { 80 , 80 , 180 , 255 } , clr :: btn_border
            ) ;
        }


        for (auto *m: all_menus(st)) {
            ui::menu_draw(st.renderer, *m,
                          st.fonts.medium,
                          clr::menu_title, clr::menu_border,
                          clr::panel_fill, clr::panel_border,
                          clr::item_fill, clr::item_border,
                          clr::item_dis);
        }


        dbg::logger_render(st.renderer, st.logger, st.fonts.small);
        gfx::editor_render(st.renderer, st.img_editor, st.fonts.medium);
        myblock :: editor_render ( st.renderer , st.my_block_ed , st.func_store , st.fonts.medium ) ;
        if ( st.context_menu_open ) {
            SDL_Rect r = st.context_menu_rect ;
            SDL_SetRenderDrawColor ( st.renderer , 45 , 45 , 45 , 255 ) ;
            SDL_RenderFillRect ( st.renderer , &r ) ;
            SDL_SetRenderDrawColor ( st.renderer , 100 , 100 , 100 , 255 ) ;
            SDL_RenderDrawRect ( st.renderer , &r ) ;
            fnt :: draw_text_left ( st.renderer , st.fonts.medium , "Delete Block" , r , { 220 , 80 , 80 , 255 } ) ;

        }


        if ( st.sprite_mgr.active >= 0 && (int)st.info_inputs.size() >= 4 ) {
            SDL_Rect panel = st.lay.spriteInfo ;

            auto draw_field = [&]( const char* lbl , int idx , int lx , int ly ) {
                SDL_Rect lr { panel.x + lx , panel.y + ly , 30 , 18 } ;
                fnt::draw_text_left ( st.renderer , st.fonts.small , lbl , lr , { 150 , 150 , 150 , 255 } ) ;
                auto & inp = st.info_inputs[idx] ;
                SDL_Rect vr = inp.rect ;
                SDL_Color bg = inp.focused ? SDL_Color { 55 , 55 , 90 , 255 } : SDL_Color { 45 , 45 , 45 , 255 } ;
                SDL_Color border = inp.focused ? SDL_Color { 100 , 140 , 255 , 255 } : SDL_Color{ 80 , 80 , 80 , 255 } ;
                SDL_SetRenderDrawColor ( st.renderer , bg.r,bg.g,bg.b,255 ) ;
                SDL_RenderFillRect ( st.renderer , &vr ) ;
                SDL_SetRenderDrawColor ( st.renderer , border.r , border.g , border.b , 255 ) ;
                SDL_RenderDrawRect ( st.renderer , &vr ) ;
                std::string display = inp.value ;
                if ( inp.focused ) display += "|" ;
                fnt :: draw_text_left ( st.renderer , st.fonts.small , display.c_str() , vr , {220,220,220,255} ) ;
            } ;

            draw_field ( "x" , 0 , 4 , 8 ) ;
            draw_field ( "y" , 1 , 124 , 8 ) ;
            draw_field ( "Size" , 2 , 4 , 38 ) ;
            draw_field ( "Dir" , 3 , 124 , 38 ) ;

            if ( st.sprite_mgr.active >= 0 ) {
                gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                SDL_Rect show_btn { panel.x + panel.w - 120 , panel.y + 8 , 50 , 20 } ;
                SDL_Rect hide_btn { panel.x + panel.w - 64  , panel.y + 8 , 50 , 20 } ;

                SDL_SetRenderDrawColor ( st.renderer , s.visible ? 30:50 , s.visible ? 140:50 , 30 , 255 ) ;
                SDL_RenderFillRect ( st.renderer , &show_btn ) ;
                SDL_SetRenderDrawColor ( st.renderer , 80,80,80,255 ) ;
                SDL_RenderDrawRect ( st.renderer , &show_btn ) ;
                fnt :: draw_text_centered ( st.renderer , st.fonts.small , "Show" , show_btn , { 255,255,255,255 } ) ;

                SDL_SetRenderDrawColor ( st.renderer , !s.visible ? 140:50 , 30 , 30 , 255 ) ;
                SDL_RenderFillRect ( st.renderer , &hide_btn ) ;
                SDL_SetRenderDrawColor ( st.renderer , 80,80,80,255 ) ;
                SDL_RenderDrawRect ( st.renderer , &hide_btn ) ;
                fnt :: draw_text_centered ( st.renderer , st.fonts.small , "Hide" , hide_btn , { 255,255,255,255 } ) ;
            }
        }



        SDL_RenderPresent(st.renderer);

    }


    int run(const runtimeConfig &cfg) {
        app_state st{};

        if (!init_sdl(st.window, st.renderer, cfg)) {
            return 1;
        }

        gfx::pen_init(st.pen);

        st.backdrops = gfx::backdrop_make_defaults();

        const char * default_backdrops[] = {
                "assets/backdrop1.jpg" ,
                "assets/backdrop2.jpg" ,
                "assets/backdrop3.jpg" ,
        };
        for ( const char * path : default_backdrops ) {
            int w = 0 , h = 0 ;
            SDL_Texture * tex = gfx :: load_texture ( st.renderer , path , w , h ) ;
            if ( tex ) {
                gfx :: backdrop_add_texture ( st.backdrops , path , tex , w , h ) ;
            }
        }

        snd::sound_init();

        fnt::font_init(st.fonts, "assets/arial.ttf");





        st.btn_run.on_click = [&] {
            st.interp.stage_w = st.lay.stage.w;
            st.interp.stage_h = st.lay.stage.h;
            compiler :: free_compiled ( st.compiled_blocks ) ;
            st.compiled_blocks = compiler :: compile_workspace ( st.workspace ) ;
            if ( !st.compiled_blocks.empty() ) {
                if ( st.sprite_mgr.active >= 0 &&
                     st.sprite_mgr.active < (int)st.sprite_mgr.sprites.size() ) {
                    st.interp.active_sprite = &st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                    st.interp.keyboard_state = SDL_GetKeyboardState(nullptr);
                    st.interp.active_stage = { st.lay.stage.x, st.lay.stage.y, st.lay.stage.w, st.lay.stage.h };
                    st.interp.timer_start = SDL_GetTicks();
                }
                core :: interpreter_load ( st.interp , st.compiled_blocks ) ;
                core :: logger_clear ( st.interp.log ) ;
                st.interp_log_forwarded = 0 ;
                st.interp.store.variables.clear();
                core :: interpreter_run ( st.interp ) ;
                dbg :: logger_log ( st.logger , "Program started." ) ;
            } else {
                dbg :: logger_log ( st.logger , "No blocks to run." , dbg :: log_level :: warn ) ;
            }
        } ;


        st.btn_stop.on_click = [&] {
            core::interpreter_stop(st.interp);
            compiler :: free_compiled ( st.compiled_blocks ) ;
            dbg::logger_log(st.logger, "Stopped.");
            core :: logger_clear ( st.interp.log ) ;
            st.interp_log_forwarded = 0 ;
        };

        st.btn_pause.on_click = [&] {
            if ( st.interp.is_paused ) {
                core :: interpreter_resume ( st.interp ) ;
                dbg :: logger_log ( st.logger , "Resumed." ) ;
            } else {
                core :: interpreter_pause ( st.interp ) ;
                dbg :: logger_log ( st.logger , "Paused." ) ;
            }
        } ;

        st.btn_step.on_click = [&] {
            if ( !st.step_mode ) {
                return ;
            }

            if ( !st.interp.running ) {
                compiler :: free_compiled ( st.compiled_blocks ) ;
                st.compiled_blocks = compiler :: compile_workspace ( st.workspace ) ;
                if ( st.compiled_blocks.empty() ) {
                    dbg :: logger_log ( st.logger , "No blocks to run." , dbg :: log_level :: warn ) ;
                    return ;
                }
                if ( st.sprite_mgr.active >= 0 && st.sprite_mgr.active < (int)st.sprite_mgr.sprites.size() ) {
                    st.interp.active_sprite = &st.sprite_mgr.sprites[st.sprite_mgr.active] ;
                    st.interp.pen = &st.pen ;
                }
                core :: interpreter_load ( st.interp , st.compiled_blocks ) ;
                st.interp.running = true ;
            }

            if ( st.interp.line_number >= (int)st.interp.blocks.size() ) {
                dbg :: logger_log ( st.logger , "Program ended. Press Step to restart." ) ;
                core :: interpreter_stop ( st.interp ) ;
                compiler :: free_compiled ( st.compiled_blocks ) ;
                return ;
            }

            core :: interpreter_step ( st.interp ) ;

            char buf[64] ;
            snprintf ( buf , sizeof(buf) , "Step %d / %d" ,
                       st.interp.line_number ,
                       (int)st.interp.blocks.size() ) ;
            dbg :: logger_log ( st.logger , buf ) ;

        } ;


        build_menus(st);

        int ww = 0, wh = 0;
        SDL_GetWindowSize(st.window, &ww, &wh);
        st.lay = ui::build_layout(ww, wh);

        gfx::sprite s = gfx::sprite_make(0, "Sprite1");
        s.draggable = true;
        int w = 0, h = 0;
        SDL_Texture *tex = gfx::load_texture(st.renderer, "assets/default_sprite.png", w, h);
        if (tex) {
            gfx::sprite_add_costume(s, tex, w, h, "default" , "assets/default_sprite.png");
        }

        gfx::sprite_set_position(s, st.lay.stage.w * 0.5f, st.lay.stage.h * 0.5f ) ;
        gfx::sprite_manager_add(st.sprite_mgr, s);
        gfx::sprite_manager_select(st.sprite_mgr, 0);

        st.interp.sound_manager = & st.sounds ;

        snd :: sound_add ( st.sounds , "assets/cat_meow.wav" , "Meow" ) ;

        while (st.running) {
            ww = 0, wh = 0;
            SDL_GetWindowSize(st.window, &ww, &wh);
            st.lay = ui::build_layout(ww, wh);

            update_rects(st);
            handle_events(st);

            SDL_GetMouseState(&st.interp.mouse_x, &st.interp.mouse_y);
            st.interp.mouse_down = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(1)) != 0 ;

            if ( !st.step_mode ) {
                core :: interpreter_tick ( st.interp ) ;
            }

            for ( int i = st.interp_log_forwarded ; i < (int)st.interp.log.entries.size() ; ++i ) {
                const auto & e = st.interp.log.entries[i] ;
                std :: string msg = "[Line:" + std :: to_string ( e.line ) + "] "
                                    + e.command + " " + e.data ;
                dbg :: log_level lvl = dbg :: log_level :: info ;
                if ( e.level == core :: log_level :: warning ) lvl = dbg :: log_level :: warn ;
                if ( e.level == core :: log_level :: error   ) lvl = dbg :: log_level :: error ;
                dbg :: logger_log ( st.logger , msg , lvl ) ;
            }
            st.interp_log_forwarded = (int)st.interp.log.entries.size() ;
            int raw_mx, raw_my;
            Uint32 mouse_state = SDL_GetMouseState(&raw_mx, &raw_my);
            st.interp.mouse_x = raw_mx - st.lay.stage.x;
            st.interp.mouse_y = raw_my - st.lay.stage.y;
            st.interp.mouse_down = (mouse_state & SDL_BUTTON(1)) != 0;
            st.interp.stage_x = st.lay.stage.x;
            st.interp.stage_y = st.lay.stage.y;
            const Uint8* ks = SDL_GetKeyboardState(nullptr);
            for (int i = 0; i < 512; i++) st.interp.keys[i] = ks[i];
            render_frame(st);
        }

        snd::sound_stop_all(st.sounds);
        snd::sound_quit();
        fnt::font_quit(st.fonts);

        shutdown_sdl(st.window, st.renderer);
        return 0;
    }
}