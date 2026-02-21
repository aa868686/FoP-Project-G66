#include "block_compiler.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace compiler {


    bool parse_number ( const std :: string & token , core :: Value & out ) {
        if ( token.empty() ) {
            return false;
        }

        char * end_i = nullptr ;
        long iv = std :: strtol ( token.c_str() , & end_i , 10 ) ;
        if ( end_i != token.c_str() && *end_i == '\0' ) {
            out = core :: value_make_int ( static_cast <int> ( iv ) ) ;
            return true ;
        }


        char * end_f = nullptr ;
        double fv = std :: strtod ( token.c_str() , &end_f ) ;
        if ( end_f != token.c_str() && *end_f == '\0' ) {
            out = core :: value_make_float ( static_cast <float> ( fv ) ) ;
            return true ;
        }

        return false ;
    }


    static std :: vector < core :: Value > extract_numbers ( const std :: string & label ) {
        std :: vector < core :: Value > nums ;
        std :: istringstream ss ( label ) ;
        std :: string tok ;

        while ( ss >> tok ) {

            while ( !tok.empty() &&
                    ( tok.back() == ',' || tok.back() == ':' || tok.back() == '%' ) ) {
                tok.pop_back() ;
            }

            core :: Value v ;

            if ( parse_number ( tok , v ) ) {
                nums.push_back ( v ) ;
            }
        }

        return nums ;
    }

    core :: Value extract_first_number ( const std :: string & label ) {
        auto nums = extract_numbers ( label ) ;
        if ( !nums.empty() ) return nums[0] ;
        return core :: value_make_int ( 0 ) ;
    }

    core :: Value extract_second_number ( const std :: string & label ) {
        auto nums = extract_numbers ( label ) ;
        if ( nums.size() >= 2 ) return nums[1] ;
        return core :: value_make_int ( 0 ) ;
    }


    static std :: string lower ( const std :: string & s ) {
        std :: string r = s ;
        std :: transform ( r.begin() , r.end() , r.begin() , :: tolower ) ;
        return r ;
    }



    core :: Block * compile_block ( const ui :: ui_block & ub ) {
        const std :: string lbl = lower ( ub.label ) ;
        auto * b = new core :: Block {} ;

        if ( lbl.find ( "move" ) != std :: string :: npos && lbl.find ( "step" ) != std :: string :: npos ) {
            b->type = core :: block_type :: move ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "turn" ) != std :: string :: npos && lbl.find ( "degree" ) != std :: string :: npos ) {
            b->type = core :: block_type :: turn ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }



        if ( lbl.find ( "go to" ) != std :: string :: npos ) {
            b->type = core :: block_type :: go_to_xy ;
            core :: Parameter px {} , py {} ;
            auto nums = extract_numbers ( ub.label ) ;
            px.data = nums.size() >= 1 ? nums[0] : core :: value_make_int ( 0 ) ;
            py.data = nums.size() >= 2 ? nums[1] : core :: value_make_int ( 0 ) ;
            b->parameters.push_back ( px ) ;
            b->parameters.push_back ( py ) ;
            return b ;
        }

        if ( lbl.find ( "point in direction" ) != std :: string :: npos ) {
            b->type = core :: block_type :: point_in_direction ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "say" ) != std :: string :: npos ) {
            b->type = core :: block_type :: say ;

            core :: Parameter p {} ;
            auto q1 = ub.label.find ( '"' ) ;
            auto q2 = ub.label.rfind ( '"' ) ;
            if ( q1 != std :: string :: npos && q2 != q1 ) {
                p.data = core :: value_make_string ( ub.label.substr ( q1+1 , q2-q1-1 ) ) ;
            } else {
                p.data = core :: value_make_string ( ub.label ) ;
            }
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "show" ) != std :: string :: npos ) {
            b->type = core :: block_type :: show ;
            return b ;
        }

        if ( lbl.find ( "hide" ) != std :: string :: npos ) {
            b->type = core :: block_type :: hide ;
            return b ;
        }

        if ( lbl.find ( "set size" ) != std :: string :: npos ) {
            b->type = core :: block_type :: set_size ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }


        if ( lbl.find ( "wait" ) != std :: string :: npos && lbl.find ( "sec" ) != std :: string :: npos ) {
            b->type = core :: block_type :: wait ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "repeat" ) != std :: string :: npos && lbl.find ( "forever" ) == std :: string :: npos ) {
            b->type = core :: block_type :: repeat ;
            core :: Parameter p {} ;
            p.data = extract_first_number ( ub.label ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "forever" ) != std :: string :: npos ) {
            b->type = core :: block_type :: forever ;
            return b ;
        }

        if ( lbl.find ( "if" ) != std :: string :: npos &&
             lbl.find ( "else" ) == std :: string :: npos ) {
            b->type = core :: block_type :: if_then ;

            core :: Parameter p {} ;
            p.data = core :: value_make_bool ( true ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "if" ) != std :: string :: npos &&
             lbl.find ( "else" ) != std :: string :: npos ) {
            b->type = core :: block_type :: if_then_else ;
            core :: Parameter p {} ;
            p.data = core :: value_make_bool ( true ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "stop all" ) != std :: string :: npos ) {
            b->type = core :: block_type :: stop_all ;
            return b ;
        }


        if ( lbl.find ( "+" ) != std :: string :: npos ) {
            b->type = core :: block_type :: op_add ;
            auto nums = extract_numbers ( ub.label ) ;
            core :: Parameter a {} , bb2 {} ;
            a.data   = nums.size() >= 1 ? nums[0] : core :: value_make_int(0) ;
            bb2.data = nums.size() >= 2 ? nums[1] : core :: value_make_int(0) ;
            b->parameters.push_back ( a ) ;
            b->parameters.push_back ( bb2 ) ;
            return b ;
        }

        delete b ;
        return nullptr ;
    }


    std :: vector < core :: Block * > compile_workspace ( const ui :: block_workspace & ws ) {
        std :: vector < core :: Block * > result ;

        std :: vector < const ui :: ui_block * > ordered ;
        for ( const auto & b: ws.blocks ) {
            ordered.push_back ( &b ) ;
        }

        std :: sort ( ordered.begin() , ordered.end() ,
                      [] ( const ui :: ui_block * a , const ui :: ui_block * b ) {
                          if ( a->y != b->y ) return a->y < b->y ;
                          return a->x < b->x ;
                      } ) ;

        for ( const auto * ub : ordered ) {
            core :: Block * cb = compile_block ( *ub ) ;
            if ( cb ) {
                result.push_back ( cb ) ;
            }
        }

        return result ;
    }


    void free_compiled ( std :: vector < core :: Block * > & blocks ) {
        for ( auto * b : blocks ) {
            delete b ;
        }
        blocks.clear() ;
    }

}
