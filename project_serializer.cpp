#include "project_serializer.h"
#include "texture_loader.h"
#include "ui_block.h"
#include "value.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <windows.h>
#include <commdlg.h>

namespace {

    std :: string esc ( const std :: string & s ) {
        std :: string r ;
        r.reserve ( s.size() ) ;
        for ( char c : s ) {
            if ( c == '\\' ) {
                r += "\\\\";
            } else if ( c == '|' ) {
                r += "\\p" ;
            } else if ( c == '\n' ) {
                r += "\\n" ;
            } else if ( c == ';' ) {
                r += "\\s" ;
            } else {
                r += c ;
            }
        }
        return r ;
    }

    std :: string unesc ( const std :: string & s ) {
        std :: string r ;
        r.reserve ( s.size() ) ;
        for ( size_t i = 0 ; i < s.size() ; ++i ) {
            if ( s[i] == '\\' && i + 1 < s.size() ) {
                ++i ;
                if ( s[i] == '\\' ) {
                    r += '\\' ;
                } else if ( s[i] == 'p'  ) {
                    r += '|'  ;
                } else if ( s[i] == 'n'  ) {
                    r += '\n' ;
                } else if ( s[i] == 's'  ) {
                    r += ';'  ;
                } else {
                    r += '\\' ; r += s[i] ;
                }
            } else {
                r += s[i] ;
            }
        }
        return r ;
    }

    bool read_field ( std :: istringstream & ss , std :: string & out ) {
        return std :: getline ( ss , out , '|' ) ? ( out = unesc(out) , true ) : false ;
    }

    int to_int ( const std :: string & s ) {
        try { return std :: stoi ( s ) ; } catch(...) { return 0 ; }
    }

    float to_float ( const std :: string & s ) {
        try { return std::stof ( s ) ; } catch(...) { return 0.0f ; }
    }

    bool to_bool ( const std :: string & s ) {
        return s == "1" || s == "true" ;
    }

    ui :: block_category cat_from_int ( int v ) {
        switch ( v ) {
            case 0 : return ui :: block_category :: motion ;
            case 1 : return ui :: block_category :: looks ;
            case 2 : return ui :: block_category :: control ;
            case 3 : return ui :: block_category :: events ;
            case 4 : return ui :: block_category :: operators ;
            case 5 : return ui :: block_category :: sound ;
            case 6 : return ui :: block_category :: variables ;
            case 7 : return ui :: block_category :: sensing ;
            default: return ui :: block_category :: motion ;
        }
    }

    int cat_to_int ( ui :: block_category c ) {
        return static_cast <int> ( c ) ;
    }

    std :: string value_type_str ( core :: value_type t ) {
        switch ( t ) {
            case core :: value_type :: type_int : return "int" ;
            case core :: value_type :: type_float : return "float" ;
            case core :: value_type :: type_string : return "string" ;
            case core :: value_type :: type_bool : return "bool" ;
        }
        return "int" ;
    }

    core :: value_type value_type_from_str ( const std :: string & s ) {
        if ( s == "float" ) {
            return core :: value_type :: type_float  ;
        }
        if ( s == "string" ) {
            return core :: value_type :: type_string ;
        }
        if ( s == "bool" ) {
            return core :: value_type :: type_bool   ;
        }
        return core :: value_type :: type_int ;
    }

}

namespace serial {

    save_result project_save (
            const std :: string & filepath ,
            const gfx :: sprite_manager & sprite_mgr ,
            const gfx :: backdrop_manager & backdrops ,
            const ui :: block_workspace & workspace ,
            const core :: variable_store & variables ,
            const snd :: sound_manager & sounds
    ) {
        std :: ofstream f ( filepath ) ;
        if ( !f.is_open() ) {
            return { false , "Cannot open file for writing: " + filepath } ;
        }

        f << "[PROJECT]\n" ;
        f << "version=1\n\n" ;

        f << "[SPRITE count=" << sprite_mgr.sprites.size() << "]\n" ;
        f << "active=" << sprite_mgr.active << "\n" ;

        for ( const auto & s : sprite_mgr.sprites ) {

            f << s.id << "|"
              << esc(s.name) << "|"
              << s.x << "|"
              << s.y << "|"
              << s.direction_deg << "|"
              << s.size_percent << "|"
              << (int) s.visible << "|"
              << (int) s.draggable << "|"
              << s.current_costume << "|"
              << esc ( s.say_text ) << "|"
              << (int) s.say_visible << "|"
              << (int) s.think_bubble << "|"
              << s.costumes.size() << "\n" ;

            for ( const auto & c : s.costumes ) {
                f << esc ( c.name ) << "," << esc ( c.filepath ) << ";" ;
            }
            if ( !s.costumes.empty() ) f << "\n" ;
        }
        f << "\n" ;

        f << "[BACKDROP]\n" ;
        f << "active=" << backdrops.active << "\n" ;
        f << "count="  << backdrops.backdrops.size() << "\n" ;
        for ( const auto & b : backdrops.backdrops ) {
            const char * type = b.texture ? "texture" : "color" ;
            f << esc ( b.name ) << "|"
              << type << "|"
              << (int) b.color.r << "," << (int)b.color.g << ","
              << (int) b.color.b << "|"
              << esc ( b.name ) << "\n" ;
        }
        f << "\n" ;

        f << "[WORKSPACE]\n" ;
        f << "next_id=" << workspace.next_id << "\n" ;
        f << "count=" << workspace.blocks.size() << "\n" ;

        for ( const auto & b : workspace.blocks ) {
            f << b.id << "|"
              << esc(b.label) << "|"
              << cat_to_int(b.category) << "|"
              << b.x << "|"
              << b.y << "|"
              << b.w << "|"
              << b.h << "|"
              << b.snap_to << "|"
              << b.parent_id << "|"
              << (int)b.is_container << "|"
              << b.container_h << "|" ;

            f << b.children.size() << "|" ;
            for ( int cid : b.children ) {
                f << cid << " " ;
            }
            f << "|" ;

            f << b.inputs.size() << "|" ;
            for ( const auto & inp : b.inputs ) {
                f << esc(inp.value) << " " ;
            }
            f << "\n" ;
        }
        f << "\n" ;


        f << "[VARIABLE count=" << variables.variables.size() << "]\n" ;
        for ( const auto & kv : variables.variables ) {
            const auto & v = kv.second.value ;
            f << esc ( kv.first ) << "|"
              << value_type_str ( v.type ) << "|"
              << esc ( core :: value_to_string ( v ) ) << "\n" ;
        }
        f << "\n" ;


        f << "[SOUND count=" << sounds.sounds.size() << "]\n" ;
        f << "active_for_sprite=" << sounds.active_for_sprite << "\n" ;
        for ( const auto & se: sounds.sounds ) {
            f << esc ( se.name ) << "|"
              << esc ( se.filepath ) << "|"
              << se.volume << "|"
              << (int) se.muted << "\n" ;
        }

        f.flush() ;
        return { true , "" } ;
    }

    load_result project_load (
            const std :: string & filepath ,
            gfx :: sprite_manager & sprite_mgr ,
            gfx :: backdrop_manager & backdrops ,
            ui :: block_workspace & workspace ,
            core :: variable_store & variables ,
            snd :: sound_manager & sounds ,
            SDL_Renderer * renderer
    ) {
        std :: ifstream f ( filepath ) ;
        if ( !f.is_open() ) {
            return { false , "Cannot open file: " + filepath } ;
        }

        sprite_mgr = gfx :: sprite_manager {} ;
        backdrops = gfx :: backdrop_make_defaults () ;
        workspace = ui :: block_workspace {} ;
        variables = core :: variable_store {} ;
        sounds.sounds.clear() ;

        std :: string line ;
        std :: string section ;

        int sprite_count = 0 ;
        int sprite_active = -1 ;
        int backdrop_count = 0 ;
        int ws_count = 0 ;
        int var_count = 0 ;
        int sound_count = 0 ;
        int sprites_read = 0 ;
        int backdrop_read = 0 ;
        int ws_read = 0 ;
        int var_read = 0 ;
        int sound_read = 0 ;

        bool next_is_costumes = false ;

        while ( std :: getline ( f , line ) ) {
            if ( !line.empty() && line.back() == '\r' ) line.pop_back() ;
            if ( line.empty() ) continue ;

            if ( line[0] == '[' ) {
                section = line ;
                next_is_costumes = false ;

                auto get_count = [&line]( const std::string & key ) -> int {
                    auto pos = line.find( key + "=" ) ;
                    if ( pos == std :: string :: npos ) {
                        return 0 ;
                    }
                    return std :: stoi ( line.substr ( pos + key.size() + 1 ) ) ;
                } ;

                if ( line.find("[SPRITE") != std :: string :: npos )
                    sprite_count = get_count("count") ;
                if ( line.find("[VARIABLE") != std :: string :: npos )
                    var_count = get_count("count") ;
                if ( line.find("[SOUND") != std :: string :: npos )
                    sound_count = get_count("count") ;
                continue ;
            }


            if ( line.find('=') != std :: string :: npos &&
                 line.find('|') == std :: string :: npos &&
                 !next_is_costumes ) {

                std :: string key = line.substr( 0 , line.find('=') ) ;
                std :: string val = line.substr( line.find('=') + 1 ) ;

                if ( section.find("[SPRITE")   != std::string::npos && key == "active" )
                    sprite_active = to_int(val) ;

                if ( section.find("[BACKDROP") != std :: string :: npos ) {
                    if ( key == "active" ) {
                        backdrops.active = to_int ( val ) ;
                    } else if ( key == "count" ) {
                        backdrop_count = to_int( val ) ;
                        backdrops.backdrops.clear() ;
                    }
                }

                if ( section.find("[WORKSPACE") != std :: string :: npos ) {
                    if ( key == "next_id" ) workspace.next_id = to_int(val) ;
                    if ( key == "count" ) ws_count = to_int ( val ) ;
                }

                if ( section.find("[SOUND") != std :: string :: npos && key == "active_for_sprite" )
                    sounds.active_for_sprite = to_int ( val ) ;

                continue ;
            }


            if ( section.find("[SPRITE") != std :: string :: npos ) {

                if ( next_is_costumes ) {
                    next_is_costumes = false ;

                    if ( !sprite_mgr.sprites.empty() ) {
                        auto & s = sprite_mgr.sprites.back() ;
                        std :: istringstream css ( line ) ;
                        std :: string entry ;
                        while ( std :: getline ( css , entry , ';' ) ) {
                            if ( entry.empty() ) {
                                continue ;
                            }

                            std :: istringstream es ( entry ) ;
                            std :: string cname , cpath ;
                            std :: getline ( es , cname , ',' ) ;
                            std :: getline ( es , cpath ) ;
                            cname = unesc ( cname ) ;
                            cpath = unesc ( cpath ) ;

                            gfx :: Costume c {} ;
                            c.name = cname ;
                            c.filepath = cpath ;

                            if ( renderer ) {
                                std :: string load_path = cpath.empty() ? "" : cpath ;
                                if ( load_path.empty() ) {
                                    load_path = "assets/default_sprite.png" ;
                                }
                                int w = 0 , h = 0 ;
                                SDL_Texture * tex = gfx :: load_texture ( renderer , load_path , w , h ) ;
                                if ( tex ) {
                                    c.texture = tex ;
                                    c.tex_w = w ;
                                    c.tex_h = h ;
                                }
                            }
                            s.costumes.push_back( c ) ;
                        }
                    }
                    continue ;
                }

                if ( sprites_read < sprite_count ) {
                    std :: istringstream ss ( line ) ;
                    std :: string f0 , f1 , f2 , f3 , f4 , f5 , f6 , f7 , f8 , f9 , f10 , f11 , f12 ;
                    if ( !( std :: getline ( ss , f0 , '|' ) && std :: getline ( ss , f1 , '|' ) &&
                            std :: getline ( ss , f2 , '|' ) && std :: getline (ss,f3,'|' ) &&
                            std :: getline ( ss , f4 , '|' ) && std :: getline ( ss , f5 , '|' ) &&
                            std :: getline ( ss , f6 , '|' ) && std :: getline ( ss , f7 , '|' ) &&
                            std :: getline ( ss , f8 , '|' ) && std :: getline ( ss , f9 , '|' ) &&
                            std :: getline ( ss , f10 , '|' ) && std :: getline ( ss , f11 , '|' ) &&
                            std :: getline ( ss , f12 )
                    ) ) {
                        continue ;
                    }

                    gfx :: sprite s = gfx :: sprite_make ( to_int ( f0 ) , unesc ( f1 ).c_str() ) ;
                    s.x = to_float(f2) ;
                    s.y = to_float(f3) ;
                    s.direction_deg = to_float ( f4 ) ;
                    s.size_percent = to_float ( f5 ) ;
                    s.visible = to_bool ( f6 ) ;
                    s.draggable = to_bool ( f7 ) ;
                    s.current_costume = to_int ( f8 ) ;
                    s.say_text = unesc ( f9 ) ;
                    s.say_visible = to_bool ( f10 ) ;
                    s.think_bubble = to_bool ( f11 ) ;

                    sprite_mgr.sprites.push_back(s) ;
                    ++sprites_read ;
                    next_is_costumes = ( to_int(f12) > 0 ) ;
                    continue ;
                }
            }

            if ( section.find ( "[BACKDROP" ) != std :: string :: npos &&
                 backdrop_read < backdrop_count ) {

                std :: istringstream ss ( line ) ;
                std :: string n, t, rgb, path ;
                if ( !( std :: getline ( ss , n , '|' ) && std :: getline ( ss , t , '|' ) &&
                        std :: getline ( ss , rgb , '|' ) && std :: getline ( ss , path ) ) ) {
                    continue ;
                }

                n = unesc( n ) ;
                path = unesc( path ) ;

                gfx :: backdrop b {} ;
                b.name = n ;

                {
                    std :: istringstream rs(rgb) ;
                    std :: string r,g,bv ;
                    std :: getline (rs ,r , ',' ) ;
                    std :: getline ( rs ,g , ',' ) ;
                    std :: getline ( rs , bv ) ;
                    b.color = { (Uint8) to_int ( r ) , (Uint8) to_int ( g ) , (Uint8) to_int ( bv ) ,255 } ;
                }

                if ( t == "texture" && renderer && !path.empty() ) {
                    int w = 0 , h = 0 ;
                    SDL_Texture* tex = gfx :: load_texture ( renderer , path , w , h ) ;
                    if ( tex ) {
                        b.texture = tex ;
                        b.tex_w = w   ;
                        b.tex_h = h   ;
                    }
                }

                backdrops.backdrops.push_back(b) ;
                ++backdrop_read ;
                continue ;
            }

            if ( section.find("[WORKSPACE") != std::string::npos &&
                 ws_read < ws_count ) {

                std :: istringstream ss ( line ) ;

                std :: string f_id , f_lbl , f_cat , f_x , f_y , f_w , f_h , f_snap , f_par , f_con , f_conh , f_cc , f_ic ;

                if ( !( std::getline(ss,f_id,'|') && std::getline(ss,f_lbl,'|') &&
                        std::getline(ss,f_cat,'|') && std::getline(ss,f_x,'|') &&
                        std::getline(ss,f_y,'|')  && std::getline(ss,f_w,'|') &&
                        std::getline(ss,f_h,'|')  && std::getline(ss,f_snap,'|') &&
                        std::getline(ss,f_par,'|') && std::getline(ss,f_con,'|') &&
                        std::getline(ss,f_conh,'|') && std::getline(ss,f_cc,'|') ) ) {
                    continue ;
                }

                ui::ui_block b {} ;
                b.id = to_int(f_id)   ;
                b.label = unesc(f_lbl)   ;
                b.category = cat_from_int( to_int(f_cat) ) ;
                b.x = to_int ( f_x ) ;
                b.y = to_int ( f_y ) ;
                b.w = to_int ( f_w ) ;
                b.h = to_int ( f_h ) ;
                b.snap_to = to_int ( f_snap ) ;
                b.parent_id = to_int ( f_par ) ;
                b.is_container = to_bool ( f_con ) ;
                b.container_h = to_int ( f_conh ) ;

                {
                    int cc = to_int ( f_cc ) ;
                    std :: string ids_str ;
                    std :: getline ( ss , ids_str , '|' ) ;
                    std :: istringstream ids ( ids_str ) ;
                    std :: string cid ;
                    for ( int i = 0 ; i < cc ; ++i ) {
                        if ( ids >> cid ) b.children.push_back ( to_int ( cid ) ) ;
                    }
                }

                {
                    std :: string ic_str ;
                    if ( std :: getline ( ss , ic_str , '|' ) ) {
                        int ic = to_int ( ic_str ) ;
                        std :: string vals_str ;
                        std :: getline ( ss , vals_str , '|' ) ;
                        std :: istringstream vs ( vals_str ) ;
                        std :: string vv ;
                        for ( int i = 0 ; i < ic ; ++i ) {
                            if ( vs >> vv ) {
                                ui :: block_input inp {} ;
                                inp.value = unesc ( vv ) ;
                                b.inputs.push_back( inp ) ;
                            }
                        }
                    }
                }

                workspace.blocks.push_back(b) ;
                ++ws_read ;
                continue ;
            }

            if ( section.find("[VARIABLE") != std :: string :: npos &&
                 var_read < var_count ) {

                std :: istringstream ss(line) ;
                std :: string n,t,v ;
                if ( !( std :: getline(ss,n,'|') && std :: getline (ss,t,'|') &&
                        std :: getline(ss,v) ) ) {
                    continue ;
                }

                n = unesc(n) ;
                v = unesc(v) ;
                auto vtype = value_type_from_str(t) ;
                core :: Value val {} ;
                switch ( vtype ) {
                    case core::value_type::type_int : val = core::value_make_int(to_int(v)) ; break ;
                    case core::value_type::type_float : val = core::value_make_float(to_float(v)) ; break ;
                    case core::value_type::type_bool : val = core::value_make_bool(to_bool(v)) ; break ;
                    case core::value_type::type_string : val = core::value_make_string(v) ; break ;
                }
                core::variable_set(variables , n , val) ;
                ++var_read ;
                continue ;
            }

            if ( section.find("[SOUND") != std::string::npos && sound_read < sound_count ) {

                std::istringstream ss(line) ;
                std::string n,p,vol,mut ;
                if ( !( std::getline(ss,n,'|') && std::getline(ss,p,'|') &&
                        std::getline(ss,vol,'|') && std::getline(ss,mut) ) ) {
                    continue ;
                }

                n = unesc(n) ;
                p = unesc(p) ;
                int idx = snd::sound_add( sounds , p , n ) ;
                if ( idx >= 0 ) {
                    snd::sound_set_volume( sounds.sounds[idx] , to_int(vol) ) ;
                    snd::sound_set_mute  ( sounds.sounds[idx] , to_bool(mut) ) ;
                }
                ++sound_read ;
                continue ;
            }
        }

        gfx::sprite_manager_select( sprite_mgr , sprite_active ) ;

        if ( !backdrops.backdrops.empty() ) {
            backdrops.active = std::max(0 , std::min(backdrops.active, (int)backdrops.backdrops.size()-1)) ;
        }

        return { true , "" } ;
    }

    void project_new (
            gfx::sprite_manager & sprite_mgr ,
            gfx::backdrop_manager & backdrops ,
            ui ::block_workspace & workspace ,
            core::variable_store & variables ,
            snd::sound_manager & sounds
    ) {
        sprite_mgr = gfx::sprite_manager {} ;
        backdrops = gfx::backdrop_make_defaults () ;
        workspace = ui ::block_workspace {} ;
        variables = core::variable_store {} ;
        snd::sound_stop_all(sounds) ;
        sounds.sounds.clear() ;
        sounds.selected = -1 ;
        sounds.active_for_sprite = 0 ;
    }


    std::string save_dialog () {
        char path[MAX_PATH] = {} ;

        OPENFILENAMEA ofn {} ;
        ofn.lStructSize = sizeof(ofn) ;
        ofn.lpstrFile = path ;
        ofn.nMaxFile = MAX_PATH ;
        ofn.lpstrFilter = "Scratch Project\0*.scratch\0All Files\0*.*\0" ;
        ofn.lpstrDefExt = "scratch" ;
        ofn.lpstrTitle = "Save Project" ;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR ;

        return GetSaveFileNameA(&ofn) ? std::string(path) : std::string{} ;
    }

    std::string load_dialog () {
        char path[MAX_PATH] = {} ;

        OPENFILENAMEA ofn {} ;
        ofn.lStructSize = sizeof(ofn) ;
        ofn.lpstrFile = path ;
        ofn.nMaxFile = MAX_PATH ;
        ofn.lpstrFilter = "Scratch Project\0*.scratch\0All Files\0*.*\0" ;
        ofn.lpstrTitle = "Open Project" ;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;

        return GetOpenFileNameA(&ofn) ? std::string(path) : std::string{} ;
    }

} // namespace serial