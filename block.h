#ifndef FOP_PROJECT_G66_BLOCK_H
#define FOP_PROJECT_G66_BLOCK_H
#include "value.h"
#include <string>
#include <vector>
namespace core {
    enum struct block_type {
        move, turn, go_to_xy, set_x, set_y, change_x, change_y, point_in_direction,
        wait, repeat, forever, if_then, if_then_else, wait_until, repeat_until, stop_all,
        say, think, show, hide, switch_costume, next_costume, set_size, change_size,
        when_flag_clicked, when_key_pressed, broadcast,
        set_variable, change_variable,
        play_sound, stop_all_sounds,
        ask_and_wait,
        touching_edge, touching_sprite, touching_color,
        distance_to_mouse, distance_to_sprite,
        key_pressed, mouse_down, mouse_x, mouse_y,
        timer, reset_timer,
        pen_down, pen_up, pen_erase_all, pen_stamp,
        op_add, op_sub, op_mul, op_div, op_eq, op_lt, op_gt, op_and, op_or, op_not,
        custom_define, custom_call
    };

    struct Parameter {
        Value data;
        bool is_variable = false;
        std::string variable_name = "" ;
    };

    struct Block {
        block_type type;
        std::vector<Parameter> parameters;
        std::vector<Block*> nested_blocks;
        std::vector<Block*> else_blocks;
        Block * condition_block = nullptr ;
        int line_number = -1;
        int jump_target = -1;
        std::string name;
    };
}
#endif //FOP_PROJECT_G66_BLOCK_H
