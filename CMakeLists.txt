cmake_minimum_required(VERSION 3.20)
project(MazeGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML COMPONENTS graphics window system REQUIRED)

add_executable(MazeGenerator MazeGenerator.cpp)

target_link_libraries(MazeGenerator SFML::Graphics SFML::Window SFML::System)

if(WIN32)
    set_target_properties(MazeGenerator PROPERTIES WIN32_EXECUTABLE TRUE)
endif()