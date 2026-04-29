cmake_minimum_required(VERSION 3.20)
project(MazeGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MazeGenerator MazeGenerator.cpp)

find_package(SFML COMPONENTS graphics window system REQUIRED)
target_link_libraries(MazeGenerator sfml-graphics sfml-window sfml-system)