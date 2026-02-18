#ifndef PROJECT_RUNTIME_H
#define PROJECT_RUNTIME_H

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace app {

    struct runtimeConfig {
        int window_w = 1280 ;
        int window_h = 720 ;
        std :: string window_title = "Scratch Engine" ;

    };

    int run ( const runtimeConfig & cfg ) ;

}

#endif //PROJECT_RUNTIME_H
