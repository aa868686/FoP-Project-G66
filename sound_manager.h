#ifndef FOP_PROJECT_G66_SOUND_MANAGER_H
#define FOP_PROJECT_G66_SOUND_MANAGER_H

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <vector>

namespace snd {

    struct sound_entry {
        std :: string name {} ;
        std :: string filepath {} ;
        Mix_Chunk *chunk = nullptr ;
        int volume = 100 ;
        bool muted = false ;
        int channel = -1 ;
    };

    struct sound_manager {
        std :: vector <sound_entry> sounds {} ;
        int selected = -1 ;
    };

    bool sound_init () ;
    void sound_quit () ;

    int sound_add ( sound_manager & sm , const std :: string & filepath ,
                    const std :: string & name
                    ) ;

    void sound_remove ( sound_manager & sm , int index ) ;

    void sound_play ( sound_entry & se ) ;
    void sound_stop ( sound_entry & se ) ;
    void sound_stop_all ( sound_manager & sm ) ;

    void sound_set_volume ( sound_entry & se , int vol ) ;
    void sound_set_mute ( sound_entry & se , bool muted ) ;


    void sound_render ( SDL_Renderer *ren ,
                        sound_manager & sm ,
                        SDL_Rect panel
                        ) ;


    bool sound_handle_click ( sound_manager & sm ,
                              SDL_Rect panel ,
                              int mx , int my
                              ) ;

    void sound_handle_drag ( sound_manager & sm ,
                             SDL_Rect panel ,
                             int mx , int my ,
                             bool mouse_down
                             ) ;

}

#endif //FOP_PROJECT_G66_SOUND_MANAGER_H
