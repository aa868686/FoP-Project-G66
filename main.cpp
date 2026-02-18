#include "runtime.h"

int main ( int /*argc*/ , char * /*argv*/[] ) {
    app :: runtimeConfig cfg ;
    cfg . window_w = 1280 ;
    cfg . window_h = 720 ;
    cfg . window_title = "Scratch Engine" ;

    return app :: run ( cfg ) ;
}
