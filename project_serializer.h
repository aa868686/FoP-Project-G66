#ifndef FOP_PROJECT_G66_PROJECT_SERIALIZER_H
#define FOP_PROJECT_G66_PROJECT_SERIALIZER_H

#pragma once

#include <string>
#include "sprite.h"
#include "backdrop.h"
#include "ui_block.h"
#include "variable.h"
#include "sound_manager.h"


namespace serial {

    struct save_result {
        bool ok = false ;
        std :: string error {} ;
    } ;

    struct load_result {
        bool ok = false ;
        std :: string error {} ;
    } ;

    save_result project_save (
            const std :: string & filepath ,
            const gfx :: sprite_manager & sprite_mgr ,
            const gfx :: backdrop_manager & backdrops ,
            const ui :: block_workspace & workspace ,
            const core :: variable_store & variables ,
            const snd :: sound_manager & sounds
    ) ;

    load_result project_load (
            const std::string & filepath ,
            gfx :: sprite_manager & sprite_mgr ,
            gfx :: backdrop_manager & backdrops ,
            ui :: block_workspace & workspace ,
            core :: variable_store & variables ,
            snd :: sound_manager & sounds ,
            SDL_Renderer * renderer
    ) ;

    void project_new (
            gfx :: sprite_manager & sprite_mgr ,
            gfx :: backdrop_manager & backdrops ,
            ui :: block_workspace & workspace ,
            core :: variable_store & variables ,
            snd :: sound_manager & sounds
    ) ;

    std :: string save_dialog () ;
    std :: string load_dialog () ;

}

#endif //FOP_PROJECT_G66_PROJECT_SERIALIZER_H
