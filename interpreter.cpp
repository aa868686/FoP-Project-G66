#include "interpreter.h"
#include "variable.h"
#include "sprite.h"
#include <chrono>
#include <thread>
#include <algorithm>
#include <SDL2/SDL.h>
namespace core {
    void interpreter_load(interpreter& interp, const std::vector<Block*>& blocks) {
        interp.blocks = blocks;
        interp.line_number = 0;
        interp.running = false;
        interp.is_paused = false;
    }
    void interpreter_pause(interpreter& interp) {
        interp.is_paused = true;
    }
    void interpreter_resume(interpreter& interp) {
        interp.is_paused = false;
    }
    void interpreter_stop(interpreter& interp) {
        interp.running = false;
        interp.is_paused = false;
        interp.line_number = 0;
        interp.blocks.clear() ;
    }
    void interpreter_run ( interpreter& interp) {
        interp.running = true;
        interp.line_number = 0 ;
        safetynet_reset(interp.safety);
    }

    void interpreter_tick ( interpreter & interp ) {
        if ( !interp.running || interp.is_paused ) {
            return ;
        }
        if ( interp.line_number >= (int) interp.blocks.size() ) {
            interp.running = false ;
            return ;
        }
        safetynet_reset ( interp.safety ) ;
        Block * current = interp.blocks[interp.line_number] ;
        interpreter_execute_block ( interp , current ) ;
        interp.line_number++ ;
    }


    void interpreter_step ( interpreter & interp ) {
        if ( !interp.running ) {
            interp.running = true ;
            safetynet_reset ( interp.safety ) ;
        }
        if ( interp.line_number < (int) interp.blocks.size() ) {
            safetynet_reset ( interp.safety ) ;
            Block * current = interp.blocks[interp.line_number] ;
            interpreter_execute_block ( interp , current ) ;
            interp.line_number++ ;
        } else {
            interp.running = false ;
        }
    }
    static Value resolve(const interpreter& interp, const Value& v) {
        if (v.type == value_type::type_string) {
            if (variable_exists(interp.store, v.string_val)) {
                return variable_get(interp.store, v.string_val);
            }
        }
        return v;
    }
    void interpreter_execute_block(interpreter& interp, Block* block) {
        if (!block) return;

        log_entry entry;
        entry.cycle = interp.cycle++;
        entry.line = interp.line_number;
        entry.level = log_level::info;

        switch(block->type) {
            case block_type::move:     entry.command = "MOVE";     entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + " steps" : ""; break;
            case block_type::turn:     entry.command = "TURN";     entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + " deg" : ""; break;
            case block_type::go_to_xy: entry.command = "GO_TO_XY"; entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + "," + value_to_string(block->parameters[1].data) : ""; break;
            case block_type::show:     entry.command = "SHOW";     break;
            case block_type::hide:     entry.command = "HIDE";     break;
            case block_type::set_size: entry.command = "SET_SIZE"; entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + "%" : ""; break;
            case block_type::wait:     entry.command = "WAIT";     entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + "s" : ""; break;
            case block_type::repeat:   entry.command = "REPEAT";   entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + "x" : ""; break;
            case block_type::forever:  entry.command = "FOREVER";  break;
            case block_type::if_then:  entry.command = "IF";       break;
            case block_type::stop_all: entry.command = "STOP";     break;
            case block_type::say:      entry.command = "SAY";      entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) : ""; break;
            case block_type::point_in_direction: entry.command = "POINT"; entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) + " deg" : ""; break;
            case block_type::think: entry.command = "THINK"; entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) : ""; break;
            case block_type::op_add: entry.command = "ADD"; entry.data = value_to_string(block->parameters[0].data) + " + " + value_to_string(block->parameters[1].data); break;
            case block_type::op_sub: entry.command = "SUB"; break;
            case block_type::op_mul: entry.command = "MUL"; break;
            case block_type::op_div: entry.command = "DIV"; break;
            case block_type::op_eq:  entry.command = "EQ";  break;
            case block_type::op_lt:  entry.command = "LT";  break;
            case block_type::op_gt:  entry.command = "GT";  break;
            case block_type::op_and: entry.command = "AND"; break;
            case block_type::op_or:  entry.command = "OR";  break;
            case block_type::op_not: entry.command = "NOT"; break;
            case block_type::op_mod:   entry.command = "MOD";   break;
            case block_type::op_round: entry.command = "ROUND"; break;
            case block_type::op_abs:   entry.command = "ABS";   break;
            case block_type::set_variable:    entry.command = "SET_VAR";    entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) : ""; break;
            case block_type::change_variable: entry.command = "CHANGE_VAR"; entry.data = !block->parameters.empty() ? value_to_string(block->parameters[0].data) : ""; break;
            case block_type::sensing_mouse_down:  entry.command = "MOUSE_DOWN";  break;
            case block_type::sensing_key_pressed: entry.command = "KEY_PRESSED"; break;
            case block_type::if_on_edge_bounce: entry.command = "BOUNCE"; break;
            default:                   entry.command = "BLOCK";    break;
        }


        logger_log(interp.log, entry);
        if (!safetynet_check(interp.safety)) {
            interpreter_stop(interp);
            return;
        }

        switch (block->type) {
            case block_type::set_variable: {
                variable_set(interp.store, value_to_string(block->parameters[0].data), resolve(interp, block->parameters[1].data));
                break;
            }
            case block_type::change_variable: {
                Value val = variable_get(interp.store, value_to_string(block->parameters[0].data));
                variable_set(interp.store, value_to_string(block->parameters[0].data),
                             value_add(val, resolve(interp, block->parameters[1].data)));
                break;
            }
            case block_type::stop_all: {
                interpreter_stop(interp);
                break;
            }
            case block_type::if_then: {
                if (value_to_bool(resolve(interp, block->parameters[0].data))) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::if_then_else: {
                if (value_to_bool(resolve(interp, block->parameters[0].data))) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                } else {
                    for (auto b: block->else_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::forever: {
                while (interp.running && !interp.is_paused) {
                    for (auto b : block->nested_blocks) {
                        if (!interp.running || interp.is_paused) break;
                        interpreter_execute_block(interp, b);
                    }
                    SDL_PumpEvents();
                    int mx, my;
                    Uint32 ms = SDL_GetMouseState(&mx, &my);
                    interp.mouse_down = (ms & SDL_BUTTON(1)) != 0;
                    interp.mouse_x = mx - interp.stage_x;
                    interp.mouse_y = my - interp.stage_y;
                    const Uint8* ks = SDL_GetKeyboardState(nullptr);
                    for (int i = 0; i < 512; i++) interp.keys[i] = ks[i];
                }
                break;
            }
            case block_type::repeat: {
                Value v = resolve(interp, block->parameters[0].data);
                int count = value_to_int(v);
                for (int i = 0; i < count && interp.running && !interp.is_paused; i++) {
                    for (auto b : block->nested_blocks) {
                        if (!interp.running || interp.is_paused) break;
                        interpreter_execute_block(interp, b);
                    }
                    SDL_PumpEvents();
                    const Uint8* ks = SDL_GetKeyboardState(nullptr);
                    for (int i2 = 0; i2 < 512; i2++) interp.keys[i2] = ks[i2];
                }
                break;
            }
            case block_type::repeat_until: {
                while (interp.running && !interp.is_paused &&
                       !value_to_bool(resolve(interp, block->parameters[0].data))) {
                    for (auto b : block->nested_blocks) {
                        if (!interp.running || interp.is_paused) break;
                        interpreter_execute_block(interp, b);
                    }
                    SDL_PumpEvents();
                    const Uint8* ks = SDL_GetKeyboardState(nullptr);
                    for (int i = 0; i < 512; i++) interp.keys[i] = ks[i];
                }
                break;
            }
            case block_type::wait: {
                Value v = resolve(interp, block->parameters[0].data);
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(value_to_float(v) * 1000)));
                break;
            }
            case block_type::move: {
                if (!interp.active_sprite) {
                    break;
                }
                gfx::sprite_move_steps(*interp.active_sprite, value_to_float(block->parameters[0].data));
                if ( interp.pen && interp.pen->is_down ) {
                    gfx::pen_add_point ( *interp.pen ,
                                         interp.active_sprite->x - interp.active_stage.x ,
                                         interp.active_sprite->y - interp.active_stage.y ) ;
                }
                break;
            }
            case block_type::turn: {
                if (!interp.active_sprite) break;
                Value v = resolve(interp, block->parameters[0].data);
                gfx::sprite_turn_degree(*interp.active_sprite, value_to_float(v));
                break;
            }
            case block_type::point_in_direction: {
                if (!interp.active_sprite) break;
                Value v = resolve(interp, block->parameters[0].data);
                gfx::sprite_set_direction(*interp.active_sprite, value_to_float(v));
                break;
            }
            case block_type::set_x: {
                if (!interp.active_sprite) {
                    break;
                }
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->x = value_to_float(v);
                if ( interp.pen && interp.pen->is_down )
                    gfx::pen_add_point(*interp.pen, interp.active_sprite->x - interp.active_stage.x, interp.active_sprite->y - interp.active_stage.y);
                break;
            }
            case block_type::set_y: {
                if (!interp.active_sprite) {
                    break;
                }
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->y = value_to_float(v);
                if ( interp.pen && interp.pen->is_down )
                    gfx::pen_add_point(*interp.pen, interp.active_sprite->x - interp.active_stage.x, interp.active_sprite->y - interp.active_stage.y);
                break;
            }
            case block_type::change_x: {
                if (!interp.active_sprite) {
                    break;
                }
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->x += value_to_float(v);
                if ( interp.pen && interp.pen->is_down )
                    gfx::pen_add_point(*interp.pen, interp.active_sprite->x - interp.active_stage.x, interp.active_sprite->y - interp.active_stage.y);
                break;
            }
            case block_type::change_y: {
                if (!interp.active_sprite) {
                    break;
                }
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->y += value_to_float(v);
                if ( interp.pen && interp.pen->is_down )
                    gfx::pen_add_point(*interp.pen, interp.active_sprite->x - interp.active_stage.x, interp.active_sprite->y - interp.active_stage.y);
                break;
            }
            case block_type::go_to_xy: {
                if (!interp.active_sprite) {
                    break;
                }
                Value vx = resolve(interp, block->parameters[0].data);
                Value vy = resolve(interp, block->parameters[1].data);
                gfx::sprite_set_position(*interp.active_sprite, value_to_float(vx), value_to_float(vy));
                if ( interp.pen && interp.pen->is_down )
                    gfx::pen_add_point(*interp.pen, interp.active_sprite->x - interp.active_stage.x, interp.active_sprite->y - interp.active_stage.y);
                break;
            }
            case block_type::show: {
                if (block->name == "show_var") {
                    if (!block->parameters.empty()) {
                        std::string vname = value_to_string(resolve(interp, block->parameters[0].data));
                        if (variable_exists(interp.store, vname)) {
                            interp.store.variables[vname].visible = true;
                        }
                    }
                } else {
                    if (interp.active_sprite)
                        gfx::sprite_set_visible(*interp.active_sprite, true);
                }
                break;
            }
            case block_type::hide: {
                if (block->name == "hide_var") {
                    if (!block->parameters.empty()) {
                        std::string vname = value_to_string(resolve(interp, block->parameters[0].data));
                        if (variable_exists(interp.store, vname)) {
                            interp.store.variables[vname].visible = false;
                        }
                    }
                } else {
                    if (interp.active_sprite)
                        gfx::sprite_set_visible(*interp.active_sprite, false);
                }
                break;
            }
            case block_type::set_size: {
                if (!interp.active_sprite) break;
                Value v = resolve(interp, block->parameters[0].data);
                gfx::sprite_set_size(*interp.active_sprite, value_to_float(v));
                break;
            }
            case block_type::change_size: {
                if (!interp.active_sprite) break;
                Value v = resolve(interp, block->parameters[0].data);
                gfx::sprite_set_size(*interp.active_sprite, value_to_float(v) + interp.active_sprite->size_percent);
                break;
            }
            case block_type::switch_costume: {
                if (!interp.active_sprite) break;
                Value v = resolve(interp, block->parameters[0].data);
                gfx::sprite_set_costume_by_name(*interp.active_sprite, value_to_string(v).c_str());
                break;
            }
            case block_type::next_costume: {
                if (!interp.active_sprite) break;
                int next = (interp.active_sprite->current_costume + 1) % interp.active_sprite->costumes.size();
                gfx::sprite_set_costume(*interp.active_sprite, next);
                interp.active_sprite->current_costume = next;
                break;
            }
            case block_type::play_sound: {
                if (interp.sound_manager) {
                    Value v = resolve(interp, block->parameters[0].data);
                    std::string name = value_to_string(v);
                    for (auto& se : interp.sound_manager->sounds) {
                        if (se.name == name) {
                            snd::sound_play(se);
                            break;
                        }
                    }
                }
                break;
            }

            case block_type::pen_down: {
                if ( !interp.pen || !interp.active_sprite ) break ;
                gfx::pen_down ( *interp.pen ,
                                interp.active_sprite->x - interp.active_stage.x ,
                                interp.active_sprite->y - interp.active_stage.y ) ;
                break ;
            }
            case block_type::pen_up: {
                if ( !interp.pen ) break ;
                gfx::pen_up ( *interp.pen ) ;
                break ;
            }
            case block_type::pen_erase_all: {
                if ( !interp.pen ) break ;
                gfx::pen_erase_all ( *interp.pen ) ;
                break ;
            }
            case block_type::pen_stamp: {
                if ( !interp.pen || !interp.active_sprite ) break ;
                const gfx::sprite & s = *interp.active_sprite ;
                if ( s.costumes.empty() || s.current_costume >= (int)s.costumes.size() ) {
                    break ;
                }
                const gfx::Costume & c = s.costumes[s.current_costume] ;
                if ( !c.texture ) {
                    break ;
                }
                float w = c.tex_w * s.size_percent / 100.0f ;
                float h = c.tex_h * s.size_percent / 100.0f ;
                SDL_FRect dst {
                        s.x - w / 2.0f ,
                        s.y - h / 2.0f ,
                        w , h
                } ;
                SDL_FPoint center { w / 2.0f , h / 2.0f } ;
                double angle = (double)(s.direction_deg - 90.0f) ;
                gfx::pen_stamp ( *interp.pen , c.texture , dst , angle , center ) ;
                break ;
            }

            case block_type::say: {
                if (!interp.active_sprite || block->parameters.empty()) break;
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->say_text = value_to_string(v);
                interp.active_sprite->say_visible = true;
                interp.active_sprite->think_bubble = false;
                break;
            }
            case block_type::think: {
                if (!interp.active_sprite || block->parameters.empty()) break;
                Value v = resolve(interp, block->parameters[0].data);
                interp.active_sprite->say_text = value_to_string(v);
                interp.active_sprite->say_visible = true;
                interp.active_sprite->think_bubble = true;
                break;
            }
            case block_type::op_add: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                Value result = value_add(a, b);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "+" + value_to_string(b) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_sub: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                Value result = value_sub(a, b);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "-" + value_to_string(b) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_mul: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                Value result = value_mul(a, b);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "*" + value_to_string(b) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_div: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                Value result = value_div(a, b);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "/" + value_to_string(b) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_eq: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                bool result = value_eq(a, b);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "=" + value_to_string(b) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_lt: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                bool result = value_lt(a, b);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + "<" + value_to_string(b) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_gt: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                bool result = value_gt(a, b);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + ">" + value_to_string(b) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_and: {
                if (block->parameters.size() < 2) break;
                bool result = value_to_bool(resolve(interp, block->parameters[0].data)) && value_to_bool(resolve(interp, block->parameters[1].data));
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("AND -> ") + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_or: {
                if (block->parameters.size() < 2) break;
                bool result = value_to_bool(resolve(interp, block->parameters[0].data)) || value_to_bool(resolve(interp, block->parameters[1].data));
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("OR -> ") + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_not: {
                if (block->parameters.empty()) break;
                bool result = !value_to_bool(resolve(interp, block->parameters[0].data));
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("NOT -> ") + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_mod: {
                if (block->parameters.size() < 2) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value b = resolve(interp, block->parameters[1].data);
                Value result = value_mod(a, b);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(a) + " mod " + value_to_string(b) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_round: {
                if (block->parameters.empty()) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value result = value_round(a);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "round(" + value_to_string(a) + ")=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_abs: {
                if (block->parameters.empty()) break;
                Value a = resolve(interp, block->parameters[0].data);
                Value result = value_abs(a);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "abs(" + value_to_string(a) + ")=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::sensing_mouse_down: {
                bool result = interp.mouse_down;
                block->parameters.resize(1);
                block->parameters[0].data = value_make_bool(result);
                variable_set(interp.store, "mouse_down", value_make_bool(result));
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("mouse_down=") + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::sensing_key_pressed: {
                std::string key_name = value_to_string(resolve(interp, block->parameters[0].data));
                bool pressed = false;
                if (key_name == "space")      pressed = interp.keys[SDL_SCANCODE_SPACE];
                else if (key_name == "up")    pressed = interp.keys[SDL_SCANCODE_UP];
                else if (key_name == "down")  pressed = interp.keys[SDL_SCANCODE_DOWN];
                else if (key_name == "left")  pressed = interp.keys[SDL_SCANCODE_LEFT];
                else if (key_name == "right") pressed = interp.keys[SDL_SCANCODE_RIGHT];
                else if (key_name.size() == 1) {
                    SDL_Scancode sc = SDL_GetScancodeFromName(key_name.c_str());
                    if (sc != SDL_SCANCODE_UNKNOWN) pressed = interp.keys[sc];
                }
                block->parameters[0].data = value_make_bool(pressed);
                variable_set(interp.store, "key_" + key_name, value_make_bool(pressed));
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "key[" + key_name + "]=" + (pressed ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::if_on_edge_bounce: {
                if (!interp.active_sprite) break;
                gfx::sprite& s = *interp.active_sprite;

                float rw = 0, rh = 0;
                gfx::sprite_get_render_size(s, rw, rh);
                float half_rw = rw * 0.5f;
                float half_rh = rh * 0.5f;

                bool hit_x = false, hit_y = false;

                if (s.x - half_rw < 0.0f) {
                    s.x = half_rw;
                    hit_x = true;
                } else if (s.x + half_rw > (float)interp.stage_w) {
                    s.x = (float)interp.stage_w - half_rw;
                    hit_x = true;
                }

                if (s.y - half_rh < 0.0f) {
                    s.y = half_rh;
                    hit_y = true;
                } else if (s.y + half_rh > (float)interp.stage_h) {
                    s.y = (float)interp.stage_h - half_rh;
                    hit_y = true;
                }

                // reflect direction
                float rad = s.direction_deg * (float)M_PI / 180.0f;
                float dx = std::cos(rad);
                float dy = std::sin(rad);  // no negative here — match move_steps convention

                if (hit_x) dx = -dx;
                if (hit_y) dy = -dy;

                s.direction_deg = std::atan2(dy, dx) * 180.0f / (float)M_PI;
                if (s.direction_deg < 0) s.direction_deg += 360.0f;

                break;
            }
            case block_type::custom_define: {
                break ;
            }
            case block_type::custom_call: {
                for ( auto * b2: interp.blocks ) {
                    if ( b2->type == block_type::custom_define && b2->name == block->name ) {
                        for ( auto * nb: b2->nested_blocks ) {
                            interpreter_execute_block ( interp , nb ) ;
                        }
                        break ;
                    }
                }
                break ;
            }
            case block_type::touching_edge: {
                if (!interp.active_sprite) break;
                const auto & s = *interp.active_sprite;
                const auto & st = interp.active_stage;
                bool touching = (s.x <= 0 || s.x >= st.w || s.y <= 0 || s.y >= st.h);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("touching edge: ") + (touching ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::key_pressed: {
                if (block->parameters.empty()) break;
                std::string key = value_to_string(block->parameters[0].data);
                bool pressed = false;
                if (interp.keyboard_state) {
                    SDL_Scancode sc = SDL_GetScancodeFromName(key.c_str());
                    if (sc != SDL_SCANCODE_UNKNOWN) {
                        pressed = interp.keyboard_state[sc];
                    }
                }
                block->parameters[0].data = value_make_bool(pressed);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "key " + key + ": " + (pressed ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::mouse_down: {
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("mouse down: ") + (interp.mouse_down ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::mouse_x: {
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "mouse x: " + std::to_string(interp.mouse_x - interp.active_stage.x);
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::mouse_y: {
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "mouse y: " + std::to_string(interp.mouse_y - interp.active_stage.y);
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::timer: {
                float t = (SDL_GetTicks() - interp.timer_start) / 1000.0f;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = "timer: " + std::to_string(t) + "s";
                rd.show_until = SDL_GetTicks() + 3000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::reset_timer: {
                interp.timer_start = SDL_GetTicks();
                break;
            }
        }
    }
}
