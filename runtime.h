#ifndef PROJECT_RUNTIME_H
#define PROJECT_RUNTIME_H

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace app {

    // Basic configuration for the SDL runtime.
    struct runtimeConfig {
        int window_w = 960 ;
        int window_h = 540 ;
        std :: string window_title = "Scratch Engine" ;

        // Stage rectangle inside the window
    };

    // Runs a minimal SDL loop that demonstrates:
    // - SDL init + renderer
    // - SDL_image init
    // - Create a Sprite, load texture, add as costume
    // - Render sprite
    // - Drag & drop on mouse
    // - Keyboard controls (W/S move, A/D turn)
    // - Pen integration (P toggle down/up, E erase, T stamp, C color, [/] size)

    // Returns 0 on clean exit, nin-zero on failure.

    int run_basic_sprite_demo ( const runtimeConfig & cfg ,
                                const std :: string & sprite_image_path
                                ) ;
}

#endif //PROJECT_RUNTIME_H
