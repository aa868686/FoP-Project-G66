//
// Created by aa8686 on 2/16/2026.
//

#ifndef PROJECT_SPRITE_H
#define PROJECT_SPRITE_H

#pragma once
#include <SDL2/SDL.h>
#include <SDL_image.h>

struct sprite {
    int x , y ;             // Coordinate
    int size_percent ;      // Default : 100
    int direction ;         // Degrees
    int visible ;           // 0/1
    int draggable ;         // 0/1
    SDL_Texture *texture ;
    int tex_w , tex_h ;
    SDL_Rect bounds ;       // updated every frame
};

// Create sprite from image and place at enter

sprite create_sprite ( SDL_Renderer *ren , const char *image_path , int screen_w , int screen_h ) ;
void destroy_sprite ( sprite *s ) ;
void sprite_set_position ( sprite *s , int x , int y ) ;
void sprite_set_center ( sprite *s , int screen_w , int screen_h ) ;
void sprite_set_size ( sprite *s , int percent ) ;
void sprite_set_direction ( sprite *s , int degrees ) ;
void sprite_show ( sprite *s ) ;
void sprite_hide ( sprite *s ) ;
void sprite_move_steps ( sprite *s , int steps ) ;
void sprite_update_bounds ( sprite *s ) ;
void sprite_draw ( SDL_Renderer *ren , const sprite *s ) ;
int sprite_hit_test ( const sprite *s , int mouse_x , int mouse_y ) ;







#endif //PROJECT_SPRITE_H
