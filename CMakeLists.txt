cmake_minimum_required(VERSION 4.0)
project(FoP-Project-G66)

set(CMAKE_CXX_STANDARD 20)

include_directories("C:/Program Files/JetBrains/MinGW")

add_executable(FoP-Project-G66
        sprite.h
        sprite.cpp
        texture_loader.h
        texture_loader.cpp
        runtime.h
        runtime.cpp
        pen.h
        pen.cpp
        layout.h
        layout.cpp
        ui_button.h
        ui_button.cpp
        ui_menu.h
        ui_menu.cpp
        main.cpp
        backdrop.h
        backdrop.cpp
        sound_manager.h
        sound_manager.cpp
        font_manager.h
        font_manager.cpp
        ui_block.h
        ui_block.cpp
        image_editor.h
        image_editor.cpp
        debug_logger.h
        debug_logger.cpp)

target_link_libraries(FoP-Project-G66
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)

add_custom_command(TARGET FoP-Project-G66 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:FoP-Project-G66>/assets
)