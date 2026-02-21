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

        struct sprite_info_input {
            SDL_Rect rect {} ;
            std :: string value {} ;
            bool focused = false ;
            enum field_type { field_x , field_y , field_size , field_direction } ;
            field_type target = field_x ;
        };

        ui::layout lay{};

        ui :: block_palette_state palette_state {} ;

        gfx::sprite_manager sprite_mgr{};


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
        ui::menu menu_run{};

        ui::button btn_run{};
        ui::button btn_stop{};

        bool running = true;
        bool is_paused = false;

        bool sprite_clicked = false ;
        bool sprite_dragged = false ;



        bool context_menu_open = false ;
        int context_menu_block_idx = -1 ;
        SDL_Rect context_menu_rect {} ;

        ui::menu *open_menu = nullptr;

        bool making_block = false ;
        char making_block_name[64] = {} ;

        std :: vector < sprite_info_input > info_inputs {} ;

        ui::block_workspace workspace{};
    };

    static std::vector<ui::menu *> all_menus(app_state &st) {
        return {&st.menu_file, &st.menu_help,
                &st.menu_code, &st.menu_settings,
                &st.menu_run};
    }


    static void close_all_menus(app_state &st) {
        for (auto *m: all_menus(st)) {
            ui::menu_close(*m);
        }
        st.open_menu = nullptr;
    }


    static void build_menus(app_state &st) {
        st.menu_file.title = "File";
        st.menu_file.items = {
                {"New Project",  true, [&] {/* TODO: clear workspace */}},
                {"Save Project", true, [&] {/* TODO: serialise */}},
                {"Load Project", true, [&] {/* TODO: deserialise */}},
        };

        st.menu_help.title = "Help";
        st.menu_help.items = {
                {"Debug Logger", true, [&] {
                    int ww, wh;
                    SDL_GetWindowSize(st.window, &ww, &wh);
                    dbg::logger_open(st.logger, ww, wh);
                    close_all_menus(st);
                }},
                {"Step-by-Step", true, [&] { /* TODO: toggle debug mode */ }},
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
                            gfx::sprite_add_costume(s, tex, w, h, "default");
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

        st.menu_run.title = "Run";
        st.menu_run.items = {
                {"Run",   true, [&] {

                    auto compiled = compiler::compile_workspace(st.workspace);
                    if (!compiled.empty()) {
                        if (st.sprite_mgr.active >= 0 &&
                            st.sprite_mgr.active < static_cast <int> ( st.sprite_mgr.sprites.size())) {
                            st.interp.active_sprite = &st.sprite_mgr.sprites[st.sprite_mgr.active];
                        }
                        core::interpreter_load(st.interp, compiled);
                        core::interpreter_run(st.interp);
                        compiler::free_compiled(compiled);
                        dbg::logger_log(st.logger, "Program finished.");
                    } else {
                        dbg::logger_log(st.logger, "No blocks to run.", dbg::log_level::warn);
                    }


                    close_all_menus(st);
                }},
                {"Pause", true, [&] {
                    core::interpreter_pause(st.interp);
                    dbg::logger_log(st.logger, "Resumed.");
                    close_all_menus(st);
                }},
                {"Stop",  true, [&] {
                    core::interpreter_stop(st.interp);
                    dbg::logger_log(st.logger, "Stopped.");
                    close_all_menus(st);
                }},
        };
    }


    static void update_rects(app_state &st) {
        const SDL_Rect &tb = st.lay.topBar;

        st.menu_file.title_rect = ui::topbar_menu_rect(tb, 0);
        st.menu_help.title_rect = ui::topbar_menu_rect(tb, 1);
        st.menu_code.title_rect = ui::topbar_menu_rect(tb, 2);
        st.menu_settings.title_rect = ui::topbar_menu_rect(tb, 3);
        st.menu_run.title_rect = ui::topbar_menu_rect(tb, 4);


        st.btn_run.rect = ui::topbar_right_rect(tb, 1, 60, 26);
        st.btn_stop.rect = ui::topbar_right_rect(tb, 0, 60, 26);


        ui::menu_layout(st.menu_file, st.fonts.medium);
        ui::menu_layout(st.menu_help, st.fonts.medium);
        ui::menu_layout(st.menu_code, st.fonts.medium);
        ui::menu_layout(st.menu_settings, st.fonts.medium);
        ui::menu_layout(st.menu_run, st.fonts.medium);

        st.info_inputs.clear() ;

        if ( st.sprite_mgr.active >= 0 ) {
            gfx :: sprite & s = st.sprite_mgr.sprites[st.sprite_mgr.active] ;
            SDL_Rect p = st.lay.spriteInfo ;
            char buf[16] ;

            auto make = [&]( app_state :: sprite_info_input :: field_type f , float val , int lx , int ly ) {
                app_state :: sprite_info_input inp ;
                inp.target = f ;
                inp.rect = { p.x + lx + 32 , p.y + ly - 2 , 52 , 20 } ;
                snprintf ( buf , 16 , "%.0f" , val ) ;
                inp.value = buf ;
                st.info_inputs.push_back ( inp ) ;
            } ;

            make ( app_state :: sprite_info_input :: field_x , s.x , 6 , 8 ) ;
            make ( app_state :: sprite_info_input :: field_y , s.y , 100 , 8 ) ;
            make ( app_state :: sprite_info_input :: field_size , s.size_percent , 6 , 36 ) ;
            make ( app_state :: sprite_info_input :: field_direction , s.direction_deg, 100 , 36 ) ;
        }


    }


    static void handle_events(app_state &st) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {

            if (e.type == SDL_QUIT) {
                st.running = false;
                return;
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    st.running = false;
                    return;
                }
            }

            if ( e.type == SDL_TEXTINPUT ) {
                if ( st.making_block ) {
                    if ( std :: strlen ( st.making_block_name ) < 63 ) {
                        std :: strncat ( st.making_block_name , e.text.text , 1 ) ;
                    }
                } else {
                    ui :: block_input_handle_key ( st.workspace , SDLK_UNKNOWN , e.text.text ) ;

                    for ( auto & inp : st.info_inputs ) {
                        if ( inp.focused && inp.value.size() < 6 ) {
                            inp.value += e.text.text ;
                        }
                    }
                }
            }

            if ( e.type == SDL_KEYDOWN ) {
                if ( st.making_block ) {
                    if ( e.key.keysym.sym == SDLK_BACKSPACE ) {
                        int len = std :: strlen ( st.making_block_name ) ;
                        if ( len > 0 ) st.making_block_name[len-1] = '\0' ;
                    } else if ( e.key.keysym.sym == SDLK_RETURN ) {
                        if ( std :: strlen ( st.making_block_name ) > 0 ) {
                            ui :: custom_block_add ( st.workspace , st.making_block_name ) ;
                        }
                        st.making_block = false ;
                    } else if ( e.key.keysym.sym == SDLK_ESCAPE ) {
                        st.making_block = false ;
                    }
                } else {
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
                                        case app_state :: sprite_info_input :: field_x : s.x = val ; break ;
                                        case app_state :: sprite_info_input :: field_y : s.y = val ; break ;
                                        case app_state :: sprite_info_input :: field_size : gfx :: sprite_set_size ( s , val ) ; break ;
                                        case app_state :: sprite_info_input :: field_direction : gfx :: sprite_set_direction ( s , val ) ; break ;
                                    }
                                } catch (...) {}
                            }
                            inp.focused = false ;
                            SDL_StopTextInput() ;
                        }
                    }
                }
            }


            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT) {

                const int mx = e.button.x;
                const int my = e.button.y;

                if ( st.making_block ) {
                    int ww = 0 , wh = 0 ;
                    SDL_GetWindowSize ( st.window , &ww , &wh ) ;
                    SDL_Rect dlg { ww/2 - 160 , wh/2 - 60 , 320 , 120 } ;
                    SDL_Rect ok_btn { dlg.x + dlg.w - 130 , dlg.y + dlg.h - 34 , 56 , 26 } ;
                    SDL_Rect cancel_btn { dlg.x + dlg.w - 68  , dlg.y + dlg.h - 34 , 56 , 26 } ;

                    if ( ui :: point_in_rect ( mx , my , ok_btn ) ) {
                        if ( std :: strlen ( st.making_block_name ) > 0 ) {
                            ui :: custom_block_add ( st.workspace , st.making_block_name ) ;
                        }
                        st.making_block = false ;
                    } else if ( ui :: point_in_rect ( mx , my , cancel_btn ) ) {
                        st.making_block = false ;
                    }
                    continue ;
                }


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

                ui :: block_input_handle_click ( st.workspace , st.lay.workspace , mx , my ) ;

                for ( auto & inp : st.info_inputs ) {
                    inp.focused = false ;
                }
                for ( auto & inp : st.info_inputs ) {
                    if ( ui :: point_in_rect ( mx , my , inp.rect ) ) {
                        inp.focused = true ;
                        inp.value = "" ;
                        SDL_StartTextInput () ;
                        break ;
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
                        if ( st.palette_state.selected_category == ui :: block_category :: my_blocks ) {
                            SDL_Rect make_btn {
                                st.lay.leftPanel.x + 64 ,
                                st.lay.leftPanel.y + 4 ,
                                st.lay.leftPanel.w - 88 , 32
                            } ;

                            if ( ui :: point_in_rect ( mx , my , make_btn ) ) {
                                st.making_block = true ;
                                std :: memset ( st.making_block_name , 0 , sizeof ( st.making_block_name ) ) ;
                                menu_consumed = true ;
                            } else {
                                int cy = st.lay.leftPanel.y + 40 ;
                                for ( auto & def : st.workspace.custom_blocks ) {
                                    SDL_Rect del { st.lay.leftPanel.x + st.lay.leftPanel.w - 24 ,
                                        cy + 8 , 16 , 16 } ;
                                    if ( ui :: point_in_rect ( mx , my , del ) ) {
                                        ui :: custom_block_remove ( st.workspace , def.id ) ;
                                        menu_consumed = true ;
                                        break ;
                                    }

                                    SDL_Rect r { st.lay.leftPanel.x + 84 , cy , st.lay.leftPanel.w - 88 , 32 } ;

                                    if ( ui :: point_in_rect ( mx , my , r ) ) {
                                        ui :: block_workspace_add ( st.workspace , def.name ,
                                              ui :: block_category :: my_blocks ,
                                              80 , 80 + static_cast <int> ( st.workspace.blocks.size() ) * 44 ) ;
                                        menu_consumed = true ;
                                        break ;
                                    }
                                    cy += 36 ;
                                }
                            }
                        }

                        if ( !menu_consumed ) {
                            ui :: block_category cat {} ;
                            std :: string label {} ;
                            if ( ui :: block_palette_click ( st.lay.leftPanel , mx , my , cat , label , st.palette_state ) ) {
                                ui :: block_workspace_add ( st.workspace , label , cat ,
                                                            80 , 80 + static_cast <int> ( st.workspace.blocks.size() ) * 44 ) ;

                                menu_consumed = true ;
                            }
                        }

                    }

                }

                if (!menu_consumed &&
                    ui::point_in_rect(mx, my, st.lay.workspace)) {
                    int idx = ui::block_hit_test(st.workspace, st.lay.workspace, mx, my);
                    if (idx >= 0) {
                        ui::block_drag_begin(st.workspace, idx, mx, my);
                        menu_consumed = true;
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
            gfx :: sprite_draw ( st.renderer , spr , sr ) ;
        }
        SDL_RenderSetClipRect ( st.renderer , nullptr ) ;

        gfx::sprite_manager_render(st.renderer, st.sprite_mgr, st.lay.spriteBar, st.fonts.small);

//        snd :: sound_render ( st.renderer , st.sounds , st.lay.spriteBar , st.fonts.small ) ;

        ui::button_draw(st.renderer, st.btn_run,
                        st.fonts.medium, "Run",
                        clr::btn_run, clr::btn_border);
        ui::button_draw(st.renderer, st.btn_stop,
                        st.fonts.medium, "Stop",
                        clr::btn_stop, clr::btn_border);


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

            draw_field ( "x" , 0 , 6 , 8 ) ;
            draw_field ( "y" , 1 , 100 , 8 ) ;
            draw_field ( "Size" , 2 , 6 , 36 ) ;
            draw_field ( "Dir" , 3 , 100 , 36 ) ;
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

        snd::sound_init();

        fnt::font_init(st.fonts, "assets/arial.ttf");


        st.btn_run.on_click = [&] {
            auto compiled = compiler::compile_workspace(st.workspace);
            if (!compiled.empty()) {
                if (st.sprite_mgr.active >= 0 &&
                    st.sprite_mgr.active < static_cast <int> ( st.sprite_mgr.sprites.size())) {
                    st.interp.active_sprite = &st.sprite_mgr.sprites[st.sprite_mgr.active];
                }
                core::interpreter_load(st.interp, compiled);
                core::interpreter_run(st.interp);
                compiler::free_compiled(compiled);
                dbg::logger_log(st.logger, "Program finished.");
            } else {
                dbg::logger_log(st.logger, "No blocks to run.", dbg::log_level::warn);
            }
        };
        st.btn_stop.on_click = [&] {
            core::interpreter_stop(st.interp);
            dbg::logger_log(st.logger, "Stopped.");
        };


        build_menus(st);

        int ww = 0, wh = 0;
        SDL_GetWindowSize(st.window, &ww, &wh);
        st.lay = ui::build_layout(ww, wh);

        gfx::sprite s = gfx::sprite_make(0, "Sprite1");
        s.draggable = true;
        int w = 0, h = 0;
        SDL_Texture *tex = gfx::load_texture(st.renderer, "assets/default_sprite.png", w, h);
        if (tex) {
            gfx::sprite_add_costume(s, tex, w, h, "default");
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
            render_frame(st);
        }

        snd::sound_stop_all(st.sounds);
        snd::sound_quit();
        fnt::font_quit(st.fonts);

        shutdown_sdl(st.window, st.renderer);
        return 0;
    }

}
