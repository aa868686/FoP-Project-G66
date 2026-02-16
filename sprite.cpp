//
// Created by aa8686 on 2/16/2026.
//

#include "sprite.h"
#include <cmath>

///// Create /////

sprite create_sprite ( SDL_Renderer *renderer , const char * image_path , int screen_w , int screen_h ) {
    sprite s{} ;

    s.texture = IMG_LoadTexture ( renderer , image_path ) ;

    if ( !s.texture ) {
        SDL_Log ( "Failed to load texture: %s" , IMG_GetError() ) ;
    }

    SDL_QueryTexture ( s.texture , NULL , NULL , &s.tex_w , &s.tex_h ) ;

    s.size_percent = 100 ;
    s.direction = 0 ;
    s.visible = 1 ;
    s.draggable = 1 ;

    s.x = screen_w / 2 ;
    s.y = screen_h / 2 ;

    sprite_update_bounds ( &s ) ;

    return s ;
}


///// Destroy /////

void destroy_sprite ( sprite *s ) {
    if ( s -> texture ) {
        SDL_DestroyTexture ( s -> texture ) ;
        s -> texture = NULL ;
    }
}


///// Position /////

void sprite_set_position ( sprite *s , int x , int y ) {
    s -> x = x ;
    s -> y = y ;
    sprite_update_bounds ( s ) ;
}


///// Size & Direction /////

void sprite_set_size ( sprite *s , int degrees ) {
    s -> direction = degrees ;
}


///// Visibility /////

void sprite_hide ( sprite *s ) {
    s -> visible = 0 ;
}


///// Movement ( Scratch style ) /////

void sprite_move_steps ( sprite *s , int steps ) {
    double radian = s -> direction * M_PI / 180.0 ;

    s -> x += static_cast <int> ( steps * cos ( radian ) ) ;
    s -> y -= static_cast <int> ( steps * sin ( radian ) ) ;

    sprite_update_bounds ( s ) ;
}



///// Bounds /////

void sprite_update_bounds ( sprite *s ) {
    int scaled_w = s -> tex_w * s -> size_percent / 100 ;
    int scaled_h = s -> tex_h * s -> size_percent / 100 ;

    s -> bounds . w = scaled_w ;
    s -> bounds . h = scaled_h ;

    s -> bounds . x = s -> x - ( scaled_w / 2 ) ;
    s -> bounds . y = s -> y - ( scaled_h / 2 ) ;

}



///// Draw /////

void sprite_draw ( SDL_Renderer *renderer , const sprite *s ) {
    if ( !s -> visible || !s -> texture ) {
        return ;
    }

    SDL_RenderCopyEx ( renderer , s -> texture , NULL , & s-> bounds , s -> direction , NULL , SDL_FLIP_NONE ) ;
}



///// Mouse hit test /////

int sprite_hot_test ( const sprite *s , int mouse_x , int mouse_y ) {
    return ( mouse_x >= s -> bounds . x &&
             mouse_x <= s -> bounds . x + s -> bounds . w &&
             mouse_y >= s -> bounds . y &&
             mouse_y <= s -> bounds . y + s -> bounds . h
    ) ;
}




































