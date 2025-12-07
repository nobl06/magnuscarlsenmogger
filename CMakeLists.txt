cmake_minimum_required(VERSION 3.10)

project(MagnusCarlsenMogger)

set(CMAKE_CXX_STANDARD 17)

add_executable(MagnusCarlsenMogger
    src/main.cpp
    src/debugger.cpp
    src/board.cpp
    src/move.cpp
    src/gen.cpp
    src/search.cpp
    src/zobrist.cpp
    src/tt.cpp
    src/eval/evaluate.cpp
    src/eval/psqt.cpp
    src/eval/material.cpp
    src/eval/positional.cpp
)
