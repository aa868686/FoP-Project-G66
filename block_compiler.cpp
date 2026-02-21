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

    static core::Value input_value(const ui::ui_block& ub, int index) {
        if (index < (int)ub.inputs.size() && !ub.inputs[index].value.empty()) {
            core::Value v;
            if (parse_number(ub.inputs[index].value, v)) return v;
            return core::value_make_string(ub.inputs[index].value);
        }
        return core::value_make_int(0);
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
            p.data = input_value(ub, 0);
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "turn" ) != std :: string :: npos && lbl.find ( "degree" ) != std :: string :: npos ) {
            b->type = core :: block_type :: turn ;
            core :: Parameter p {} ;
            p.data = input_value(ub, 0);
            b->parameters.push_back ( p ) ;
            return b ;
        }



        if ( lbl.find ( "go to" ) != std :: string :: npos ) {
            b->type = core :: block_type :: go_to_xy ;
            core :: Parameter px {} , py {} ;
            auto nums = extract_numbers ( ub.label ) ;
            px.data = input_value(ub, 0);
            py.data = input_value(ub, 1);
            b->parameters.push_back ( px ) ;
            b->parameters.push_back ( py ) ;
            return b ;
        }

        if ( lbl.find ( "point in direction" ) != std :: string :: npos ) {
            b->type = core :: block_type :: point_in_direction ;
            core :: Parameter p {} ;
            p.data = input_value(ub, 0);
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "say" ) != std :: string :: npos ) {
            b->type = core :: block_type :: say ;

            core :: Parameter p {} ;
            p.data = input_value(ub, 0);
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
            p.data = input_value(ub, 0);
            b->parameters.push_back ( p ) ;
            return b ;
        }


        if ( lbl.find ( "wait" ) != std :: string :: npos && lbl.find ( "sec" ) != std :: string :: npos ) {
            b->type = core :: block_type :: wait ;
            core :: Parameter p {} ;
            p.data = input_value(ub, 0);
            b->parameters.push_back ( p ) ;
            return b ;
        }

        if ( lbl.find ( "repeat" ) != std :: string :: npos && lbl.find ( "forever" ) == std :: string :: npos ) {
            b->type = core :: block_type :: repeat ;
            core :: Parameter p {} ;
            p.data = input_value(ub, 0);
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
            a.data   = input_value(ub, 0);
            bb2.data = input_value(ub, 1);
            b->parameters.push_back ( a ) ;
            b->parameters.push_back ( bb2 ) ;
            return b ;
        }

        delete b ;
        return nullptr ;
    }


    std::vector<core::Block*> compile_workspace(const ui::block_workspace& ws) {
        std::vector<core::Block*> result;

        // Step 1: compile all blocks into a map by their ui id
        std::map<int, core::Block*> compiled_map;
        std::map<int, int> snap_map; // ui_block id -> snap_to id

        for (const auto& ub : ws.blocks) {
            core::Block* cb = compile_block(ub);
            if (cb) {
                compiled_map[ub.id] = cb;
                snap_map[ub.id] = ub.snap_to;
            }
        }

        // Step 2: build parent-child relationships
        for (const auto& ub : ws.blocks) {
            if (ub.snap_to >= 0) {
                auto parent_it = compiled_map.find(ub.snap_to);
                auto child_it  = compiled_map.find(ub.id);
                if (parent_it != compiled_map.end() &&
                    child_it  != compiled_map.end()) {
                    core::Block* parent = parent_it->second;
                    core::Block* child  = child_it->second;
                    // if parent is a container block, add as nested
                    if (parent->type == core::block_type::repeat   ||
                        parent->type == core::block_type::forever  ||
                        parent->type == core::block_type::if_then  ||
                        parent->type == core::block_type::if_then_else) {
                        parent->nested_blocks.push_back(child);
                    }
                }
            }
        }

        // Step 3: top-level blocks are those with no parent (snap_to == -1)
        // sort by y position
        std::vector<const ui::ui_block*> roots;
        for (const auto& ub : ws.blocks) {
            if (ub.snap_to < 0 && compiled_map.count(ub.id)) {
                roots.push_back(&ub);
            }
        }
        std::sort(roots.begin(), roots.end(),
                  [](const ui::ui_block* a, const ui::ui_block* b) {
                      return a->y != b->y ? a->y < b->y : a->x < b->x;
                  });

        for (const auto* ub : roots) {
            result.push_back(compiled_map[ub->id]);
        }

        return result;
    }


    void free_compiled ( std :: vector < core :: Block * > & blocks ) {
        for ( auto * b : blocks ) {
            delete b ;
        }
        blocks.clear() ;
    }

}
