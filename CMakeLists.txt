cmake_minimum_required(VERSION 3.20)
project(MazeGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(WIN32)
    set(SFML_STATIC_LIBRARIES TRUE)
    add_definitions(-DSFML_STATIC)
endif()

find_package(SFML COMPONENTS graphics window system REQUIRED)

add_executable(MazeGenerator MazeGenerator.cpp)

add_custom_command(TARGET MazeGenerator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/frames
        ${CMAKE_CURRENT_BINARY_DIR}/frames
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/finish_frames
        ${CMAKE_CURRENT_BINARY_DIR}/finish_frames
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/*.png
        ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
    target_link_libraries(MazeGenerator
            sfml-graphics-s
            sfml-window-s
            sfml-system-s
            opengl32
            winmm
            gdi32
            freetype
    )
else()
    target_link_libraries(MazeGenerator
            sfml-graphics
            sfml-window
            sfml-system
    )
endif()