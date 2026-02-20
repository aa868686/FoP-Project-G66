//
// Created by amirf on 2/20/2026.
//

#include "interpreter.h"
#include "variable.h"
#include "sprite.h"
#include <chrono>
#include <thread>
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
    }
    void interpreter_run(interpreter& interp) {
        interp.running = true;
        safetynet_reset(interp.safety);
        while (interp.running && !interp.is_paused && interp.line_number < (int)interp.blocks.size()) {
            Block* current = interp.blocks[interp.line_number];
            interpreter_execute_block(interp, current);
            interp.line_number++;
        }
    }
    void interpreter_execute_block(interpreter& interp, Block* block) {
        if (!block) return;

        log_entry entry;
        entry.cycle = interp.cycle++;
        entry.line = interp.line_number;
        entry.command = "BLOCK";
        entry.level = log_level::info;
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
        }
    }
}
