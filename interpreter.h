#ifndef FOP_PROJECT_G66_INTERPRETER_H
#define FOP_PROJECT_G66_INTERPRETER_H
#include "block.h"
#include "variable.h"
#include <vector>
#include "sprite.h"
#include "sound_manager.h"
#include "logger.h"
#include "safetynet.h"
#include "ui_block.h"
#include <SDL2/SDL.h>
namespace core {

    struct result_display {
        int block_line = -1;
        std::string text = "";
        Uint32 show_until = 0;
    };

    struct interpreter {
        std::vector<Block*> blocks;
        int line_number = 0;
        variable_store store;
        bool running = false;
        bool is_paused = false;
        gfx::sprite* active_sprite = nullptr;
        logger log;
        int cycle = 0;
        safety_net safety;
        snd :: sound_manager * sound_manager = nullptr ;
        std::vector<result_display> results;
    };


    void interpreter_load(interpreter& interp, const std::vector<Block*>& blocks);
    void interpreter_run(interpreter& interp);
    void interpreter_tick ( interpreter & interp ) ;
    void interpreter_pause(interpreter& interp);
    void interpreter_resume(interpreter& interp);
    void interpreter_stop(interpreter& interp);
    void interpreter_step ( interpreter & interp ) ;
    void interpreter_execute_block(interpreter& interp, Block* block);

    }
#endif //FOP_PROJECT_G66_INTERPRETER_H
