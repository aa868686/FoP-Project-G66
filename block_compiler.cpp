#include "block_compiler.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <map>

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



    static core :: Value get_input ( const ui :: ui_block & ub , int idx ) {
        if ( idx < (int)ub.inputs.size() && !ub.inputs[idx].value.empty() ) {
            core :: Value v ;
            if ( parse_number ( ub.inputs[idx].value , v ) ) return v ;
            return core :: value_make_string ( ub.inputs[idx].value ) ;
        }
        return core :: value_make_int ( 0 ) ;
    }

    core :: Block * compile_block ( const ui :: ui_block & ub ) {
        const std :: string lbl = lower ( ub.label ) ;
        auto * b = new core :: Block {} ;

        if ( lbl.find ( "move" ) != std :: string :: npos && lbl.find ( "step" ) != std :: string :: npos ) {
            b->type = core :: block_type :: move ;
            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "turn" ) != std :: string :: npos && lbl.find ( "degree" ) != std :: string :: npos ) {
            b->type = core :: block_type :: turn ;
            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }



        if ( lbl.find ( "go to" ) != std :: string :: npos ) {
            b->type = core :: block_type :: go_to_xy ;
            core :: Parameter px {} , py {} ;
            px.data = get_input ( ub , 0 ) ;
            py.data = get_input ( ub , 1 ) ;
            b->parameters.push_back ( px ) ;
            b->parameters.push_back ( py ) ;
            return b ;
        }

        if ( lbl.find ( "point in direction" ) != std :: string :: npos ) {
            b->type = core :: block_type :: point_in_direction ;
            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "say" ) != std :: string :: npos ) {
            b->type = core :: block_type :: say ;

            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
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
            p.data = get_input ( ub , 0 ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }


        if ( lbl.find ( "wait" ) != std :: string :: npos && lbl.find ( "sec" ) != std :: string :: npos ) {
            b->type = core :: block_type :: wait ;
            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "repeat" ) != std :: string :: npos && lbl.find ( "forever" ) == std :: string :: npos ) {
            b->type = core :: block_type :: repeat ;
            core :: Parameter p {} ;
            p.data = get_input ( ub , 0 ) ;
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


    static core :: Block * compile_block_recursive ( const ui :: ui_block & ub , const ui :: block_workspace & ws ) {
        core :: Block * cb = compile_block ( ub ) ;
        if ( !cb ) return nullptr ;

        if ( ub.is_container ) {
            // Sort children by y position
            std :: vector < const ui :: ui_block * > sorted_children ;
            for ( int cid : ub.children ) {
                for ( const auto & child : ws.blocks ) {
                    if ( child.id == cid ) {
                        sorted_children.push_back ( &child ) ;
                        break ;
                    }
                }
            }
            std :: sort ( sorted_children.begin() , sorted_children.end() ,
                          [] ( const ui :: ui_block * a , const ui :: ui_block * b ) {
                              return a->y < b->y ;
                          } ) ;

            for ( const auto * child : sorted_children ) {
                core :: Block * child_cb = compile_block_recursive ( *child , ws ) ;
                if ( child_cb ) {
                    cb->nested_blocks.push_back ( child_cb ) ;
                }
            }
        }

        return cb ;
    }

    std :: vector < core :: Block * > compile_workspace ( const ui :: block_workspace & ws ) {
        std :: vector < core :: Block * > result ;

        // Only top-level blocks (no parent)
        std :: vector < const ui :: ui_block * > ordered ;
        for ( const auto & b : ws.blocks ) {
            if ( b.parent_id < 0 ) {
                ordered.push_back ( &b ) ;
            }
        }

        std :: sort ( ordered.begin() , ordered.end() ,
                      [] ( const ui :: ui_block * a , const ui :: ui_block * b ) {
                          if ( a->y != b->y ) return a->y < b->y ;
                          return a->x < b->x ;
                      } ) ;

        for ( const auto * ub : ordered ) {
            core :: Block * cb = compile_block_recursive ( *ub , ws ) ;
            if ( cb ) {
                result.push_back ( cb ) ;
            }
        }

        return result ;
    }

    static void free_block_recursive ( core :: Block * b ) {
        if ( !b ) return ;
        for ( auto * nested : b->nested_blocks ) {
            free_block_recursive ( nested ) ;
        }
        for ( auto * eb : b->else_blocks ) {
            free_block_recursive ( eb ) ;
        }
        delete b ;
    }

    void free_compiled ( std :: vector < core :: Block * > & blocks ) {
        for ( auto * b : blocks ) {
            free_block_recursive ( b ) ;
        }
        blocks.clear() ;
    }

}