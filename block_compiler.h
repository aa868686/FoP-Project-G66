#ifndef FOP_PROJECT_G66_BLOCK_COMPILER_H
#define FOP_PROJECT_G66_BLOCK_COMPILER_H

#pragma once

#include "ui_block.h"
#include "block.h"
#include "value.h"
#include <vector>
#include <string>



namespace compiler {

    bool parse_number ( const std :: string & token , core :: Value & out ) ;

    core :: Value extract_first_number ( const std :: string & label ) ;

    core :: Value extract_second_number ( const std :: string & label ) ;

    core :: Block * compile_block ( const ui :: ui_block & ub ) ;

    std :: vector < core :: Block * > compile_workspace ( const ui :: block_workspace & ws ) ;

    void free_compiled ( std :: vector < core :: Block * > & blocks ) ;


}

#endif //FOP_PROJECT_G66_BLOCK_COMPILER_H
