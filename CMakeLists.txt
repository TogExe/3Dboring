cmake_minimum_required(VERSION 3.29)
project(3Dboring)

set(CMAKE_CXX_STANDARD 20)

add_executable(3Dboring main.cpp
        dungeon.cpp
        donjon.h)

target_link_libraries(3Dboring gdiplus)
