#ifndef PROJECT_TEXTURE_LOADER_H
#define PROJECT_TEXTURE_LOADER_H

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace gfx {

    // Loads an image file into an SDL_Texture using SDL_image .
    // Returns nullptr on failure. On success, out_w/out_h are set to the image size.
    // Texture ownership is transferred to the caller.

    SDL_Texture * load_texture ( SDL_Renderer * renderer ,
                                 const std :: string & file_path ,
                                 int & out_w ,
                                 int & out_h
                                 ) ;
}

#endif //PROJECT_TEXTURE_LOADER_H
