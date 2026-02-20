#include "file_dialog.h"

#include <windows.h>
#include <commdlg.h>
#include <cstring>

namespace dlg {

    static std :: string open_dialog ( const char * filter , const char * title ) {
        char path [ MAX_PATH ] = {} ;

        OPENFILENAMEA ofn {} ;
        ofn.lStructSize = sizeof ( ofn ) ;
        ofn.hwndOwner = nullptr ;
        ofn.lpstrFile = path ;
        ofn.nMaxFile = MAX_PATH ;
        ofn.lpstrFilter = filter ;
        ofn.nFilterIndex = 1 ;
        ofn.lpstrTitle = title ;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;

        if ( GetOpenFileNameA ( &ofn ) ) {
            return std :: string ( path ) ;
        }
        return {} ;
    }


    std :: string open_image_dialog () {
        return open_dialog (
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0"
                "PNG Files\0*.png\0"
                "JPEG Files\0*.jpg;*.jpeg\0"
                "BMP Files\0*.bmp\0"
                "All Files\0*.*\0" ,
                "Open Image"
                ) ;
    }


    std :: string open_audio_dialog () {
        return open_dialog (
                "Audio Files\0*.wav;*.mp3;*.ogg\0"
                "WAV Files\0*.wav\0"
                "MP3 Files\0*.mp3\0"
                "OGG Files\0*.ogg\0"
                "All Files\0*.*\0" ,
                "Open Audio"
                ) ;
    }


}
