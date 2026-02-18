#ifndef PROJECT_RUNTIME_H
#define PROJECT_RUNTIME_H

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace app {

    // Basic configuration for the SDL runtime.
    struct runtimeConfig {
        int window_w = 1280 ;
        int window_h = 720 ;
        std :: string window_title = "Scratch Engine" ;

    };

    // Main application entry point.
    // Initialises SDL, builds the fill IDE layout, and runs the event loop.
    // Returns 0 on clean exit, non-zero on failure.

    int run_basic_sprite_demo ( const runtimeConfig & cfg ) ;

}

#endif //PROJECT_RUNTIME_H
