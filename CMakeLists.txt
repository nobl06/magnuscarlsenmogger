cmake_minimum_required(VERSION 3.10)

project(MagnusCarlsenMogger)

set(CMAKE_CXX_STANDARD 17)

add_executable(MagnusCarlsenMogger
    src/main.cpp
    src/debugger.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
    src/evaluate.cpp
    src/piece_values.cpp
)