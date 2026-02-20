//
// Created by amirf on 2/20/2026.
//

#ifndef FOP_PROJECT_G66_INTERPRETER_H
#define FOP_PROJECT_G66_INTERPRETER_H
#include "block.h"
#include "variable.h"
#include <vector>
#include "sprite.h"
namespace core {
    struct interpreter {
        std::vector<Block*> blocks;
        int line_number = 0;
        variable_store store;
        bool running = false;
        bool is_paused = false;
        gfx::sprite* active_sprite = nullptr;
    };

    void interpreter_load(interpreter& interp, const std::vector<Block*>& blocks);
    void interpreter_run(interpreter& interp);
    void interpreter_pause(interpreter& interp);
    void interpreter_resume(interpreter& interp);
    void interpreter_stop(interpreter& interp);

    void interpreter_execute_block(interpreter& interp, Block* block);
}
#endif //FOP_PROJECT_G66_INTERPRETER_H
