#ifndef FOP_PROJECT_G66_PEN_H
#define FOP_PROJECT_G66_PEN_H

#pragma once

#include <SDL2/SDL.h>
#include <vector>

namespace gfx {

    // Pen draws persistently on the stage until "Erase All" is called.

    struct PenSegment {
        float x0 = 0.0f ;
        float y0 = 0.0f ;
        float x1 = 0.0f ;
        float y1 = 0.0f ;
        SDL_Color color { 0 , 0 , 0 , 255 } ;
        int size = 1 ; // thickness in pixels
    };

    // A stamped snapshot of a sprite's current texture at a moment in time.
    // Texture ownership is external.

    struct PenStamp {
        SDL_Texture * texture = nullptr ;
        SDL_FRect dst {} ; ; // destination rectangle in screen space
        double angle_deg = 0 ; // SDL_RenderCopyExF angle (Clockwise)
        SDL_FPoint center {} ; // rotation center in dst space
    };

    struct pen_state {
        bool is_down = false ;

        // Current pen style
        SDL_Color color { 0 , 0 , 0 , 255 } ;
        int size = 1 ;

        // Internal state
        bool has_last = false ;
        float last_x = 0.0f ;
        float last_y = 0.0f ;


        // Persistent outputs
        std :: vector < PenSegment > segment ;
        std :: vector < PenStamp > stamps ;
    };

    struct StageRect {
        int x = 0 ;
        int y = 0 ;
        int w = 0 ;
        int h = 0 ;
    };


    // Initialize / reset
    void pen_init ( pen_state & pen ) ;
    void pen_erase_all ( pen_state & pen ) ;


    // Down/Up
    void pen_down ( pen_state & pen , float x , float y ) ;
    void pen_up ( pen_state & pen ) ;


    // Color and size
    void pen_set_color ( pen_state & pen , SDL_Color c ) ;
    void pen_set_size ( pen_state & pen , int size_px ) ;
    void pen_change_size ( pen_state & pen , int delta_px ) ;


    // Drawing: add a point (if pen is down and has a last point, creates a segment)
    void pen_add_point ( pen_state & pen , float  x , float y ) ;

    // Stamp: caller provides the sprite's current texture + render params
    // dst should already include stage offset (screen space).
    void pen_stamp ( pen_state & pen ,
                     SDL_Texture *texture ,
                     const SDL_FRect & dst ,
                     double angle_deg ,
                     SDL_FPoint center
                     ) ;

    void pen_render ( SDL_Renderer * ren , const pen_state & pen , const StageRect & stage ) ;



}

#endif //FOP_PROJECT_G66_PEN_H
