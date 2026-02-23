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
            default:                   entry.command = "BLOCK";    break;
        }


        logger_log(interp.log, entry);
        if (!safetynet_check(interp.safety)) {
            interpreter_stop(interp);
            return;
        }

        switch (block->type) {
            case block_type::set_variable: {
                variable_set(interp.store, value_to_string(block->parameters[0].data), block->parameters[1].data);
                break;
            }
            case block_type::change_variable: {
                Value val = variable_get(interp.store, value_to_string(block->parameters[0].data));
                variable_set(interp.store, value_to_string(block->parameters[0].data),
                             value_add(val, block->parameters[1].data));
                break;
            }
            case block_type::stop_all: {
                interpreter_stop(interp);
                break;
            }
            case block_type::if_then: {
                if (value_to_bool(block->parameters[0].data)) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::if_then_else: {
                if (value_to_bool(block->parameters[0].data)) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                else {
                    for (auto b: block->else_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::repeat: {
                for (int i = 0; i < value_to_int(block->parameters[0].data); i++) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::repeat_until: {
                while (interp.running && !value_to_bool(block->parameters[0].data)) {
                    for (auto b: block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::forever: {
                while (interp.running) {
                    for (auto b : block->nested_blocks) {
                        interpreter_execute_block(interp, b);
                    }
                }
                break;
            }
            case block_type::wait: {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(value_to_float(block->parameters[0].data)*1000)));
                break;
            }
            case block_type::move: {
                if (!interp.active_sprite) break;
                gfx::sprite_move_steps(*interp.active_sprite, value_to_float(block->parameters[0].data));
                break;
            }
            case block_type::turn: {
                if (!interp.active_sprite) break;
                gfx::sprite_turn_degree(*interp.active_sprite, value_to_float(block->parameters[0].data));
                break;
            }
            case block_type :: point_in_direction : {
                if ( !interp.active_sprite ) {
                    break ;
                }
                gfx :: sprite_set_direction ( *interp.active_sprite , value_to_float ( block -> parameters[0].data ) ) ;
                break ;
            }
            case block_type::set_x: {
                if (!interp.active_sprite) break;
                interp.active_sprite->x = value_to_float(block->parameters[0].data);
                break;
            }
            case block_type::set_y: {
                if (!interp.active_sprite) break;
                interp.active_sprite->y = value_to_float(block->parameters[0].data);
                break;
            }
            case block_type::change_x: {
                if (!interp.active_sprite) break;
                interp.active_sprite->x += value_to_float(block->parameters[0].data);
                break;
            }
            case block_type::change_y: {
                if (!interp.active_sprite) break;
                interp.active_sprite->y += value_to_float(block->parameters[0].data);
                break;
            }
            case block_type::go_to_xy: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_position(*interp.active_sprite, value_to_float(block->parameters[0].data), value_to_float(block->parameters[1].data));
                break;
            }
            case block_type::show: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_visible(*interp.active_sprite, true);
                break;
            }
            case block_type::hide: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_visible(*interp.active_sprite, false);
                break;
            }
            case block_type::set_size: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_size(*interp.active_sprite, value_to_float(block->parameters[0].data));
                break;
            }
            case block_type::change_size: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_size(*interp.active_sprite, value_to_float(block->parameters[0].data) + interp.active_sprite->size_percent);
                break;
            }
            case block_type::switch_costume: {
                if (!interp.active_sprite) break;
                gfx::sprite_set_costume_by_name(*interp.active_sprite, (value_to_string(block->parameters[0].data).c_str()));
                break;
            }
            case block_type::next_costume: {
                if (!interp.active_sprite) break;
                int next = (interp.active_sprite->current_costume+1) % interp.active_sprite->costumes.size();
                gfx::sprite_set_costume(*interp.active_sprite, next);
                interp.active_sprite->current_costume = next;
                break;
            }

            case block_type :: play_sound : {
                if ( interp.sound_manager ) {
                    std :: string name = value_to_string ( block -> parameters[0].data ) ;
                    for ( auto & se : interp.sound_manager -> sounds ) {
                        if ( se.name == name ) {
                            snd :: sound_play ( se ) ;
                            break ;
                        }
                    }
                }
                break ;
            }

            case block_type::say: {
                if (!interp.active_sprite || block->parameters.empty()) break;
                interp.active_sprite->say_text = value_to_string(block->parameters[0].data);
                interp.active_sprite->say_visible = true;
                interp.active_sprite->think_bubble = false;
                break;
            }
            case block_type::think: {
                if (!interp.active_sprite || block->parameters.empty()) break;
                interp.active_sprite->say_text = value_to_string(block->parameters[0].data);
                interp.active_sprite->say_visible = true;
                interp.active_sprite->think_bubble = true;
                break;
            }
            case block_type::op_add: {
                if (block->parameters.size() < 2) break;
                Value result = value_add(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "+" + value_to_string(block->parameters[1].data) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_sub: {
                if (block->parameters.size() < 2) break;
                Value result = value_sub(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "-" + value_to_string(block->parameters[1].data) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_mul: {
                if (block->parameters.size() < 2) break;
                Value result = value_mul(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "*" + value_to_string(block->parameters[1].data) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_div: {
                if (block->parameters.size() < 2) break;
                Value result = value_div(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = result;
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "/" + value_to_string(block->parameters[1].data) + "=" + value_to_string(result);
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_eq: {
                if (block->parameters.size() < 2) break;
                bool result = value_eq(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "=" + value_to_string(block->parameters[1].data) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_lt: {
                if (block->parameters.size() < 2) break;
                bool result = value_lt(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + "<" + value_to_string(block->parameters[1].data) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_gt: {
                if (block->parameters.size() < 2) break;
                bool result = value_gt(block->parameters[0].data, block->parameters[1].data);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = value_to_string(block->parameters[0].data) + ">" + value_to_string(block->parameters[1].data) + " -> " + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
            case block_type::op_and: {
                if (block->parameters.size() < 2) break;
                bool result = value_to_bool(block->parameters[0].data) && value_to_bool(block->parameters[1].data);
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
                bool result = value_to_bool(block->parameters[0].data) || value_to_bool(block->parameters[1].data);
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
                bool result = !value_to_bool(block->parameters[0].data);
                block->parameters[0].data = value_make_bool(result);
                result_display rd;
                rd.block_line = interp.line_number;
                rd.text = std::string("NOT -> ") + (result ? "true" : "false");
                rd.show_until = SDL_GetTicks() + 5000;
                interp.results.push_back(rd);
                break;
            }
        }
    }
}
